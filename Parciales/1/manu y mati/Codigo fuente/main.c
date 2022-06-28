#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "buffer.h"
#include "shared.h"

// Definimos colores para la consola
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// Definimos los hilos
#define THREAD 9

// Definimos las funciones de color para la consola
void red() { printf(ANSI_COLOR_RED); }
void yellow() { printf(ANSI_COLOR_YELLOW); }
void green() { printf(ANSI_COLOR_GREEN); }
void defaultColor() { printf(ANSI_COLOR_RESET); }

// Funciones para el funcionamiento básico de los hilos	
void *phone(void *arg);
void *chef(void *arg);
void *delivery(void *arg);
void *GUI(void *arg);
void *readKeyboard(void * arg);
void *countdown(void * arg);
int mainMenu();
int initialize(void *arg);
int terminate(void *arg);

struct Game{
	struct Monitor *m1;
	struct Monitor *m2;
	struct Memory *mem;
	sem_t *call;
	int phone;
	int gameTime;
	int option;
	int lostCalls;
	int takenOrders;
	int cookedOrders;
	int sentOrders;
	int paidOrders;
	int totalMoney;
	int payment;
	int id;
	int chefState[3];
	int deliveryState[2];
	int closeFlag;
};

// Hacemos nueve hilos para manejar telefonos, cocineros, delivery, GUI, keybord y countdown
int main(int argc, char *v[]){
	system("clear"); // limpiamos la consola	
	if(mainMenu() == 2) return 0;
	int money = 0;
	pthread_t *th;
	int error;
	th = (pthread_t *)(calloc(THREAD, sizeof(pthread_t)));

	struct Game *game = (struct Game *)(calloc(1, sizeof(struct Game)));

	error = initialize((void *)game);
	if(error) perror("initialize()");

	// Creamos los hilos, cocineros, delivery
	pthread_create(&th[0], NULL, phone, (void *)(game));
	for (int i = 1; i < 4; i++)
	{
		pthread_create(&th[i], NULL, chef, (void *)(game));
		usleep(50000);
		game->id++;
	}
	game->id=0;
	for (int i = 4; i < 6; i++)
	{
		pthread_create(&th[i], NULL, delivery, (void *)(game));
		usleep(50000);
		game->id++;
	}
	// fin Creamos los hilos, cocineros, delivery

	// Creamos el hilo GUI, keybord y countdown
	time_t t;
	srand((unsigned)time(&t));
	pthread_create(&th[6], NULL, GUI, (void *)(game));
	pthread_create(&th[7], NULL, readKeyboard, (void *)(game));
	pthread_create(&th[8], NULL, countdown, (void *)(game));
	int randomOrder = -1;
	while (game->gameTime > 0){

		switch(game->option){
			case 1:
				error = sem_trywait(game->call);
				if(!error){
					game->takenOrders++;
					randomOrder = (rand() % 4);
					game->option = 0;
				}
				break;
			case 2:
				if(randomOrder > -1){
					error = SaveData(game->m1, randomOrder);
				}
				randomOrder = -1;
				if (error)
					perror("SaveData()");
				game->option = 0;
				usleep(2000);
				break;
			case 3:
				error = readMemory(game->mem,&money);
				if(error) perror("readMemory()");
				if(money > 0){
					switch(money){
						case 0:
							game->totalMoney += 50;
							break;
						case 1:
							game->totalMoney += 100;
							break;
						case 2:
							game->totalMoney += 200;
							break;	
						case 3:
							game->totalMoney += 100;
							break;
						case 4:
							game->totalMoney += 400;
							break;
					}
					game->paidOrders++;
					game->payment = 0;
					money = 0;
					game->option = 0;
				}
				break;
			case 0:
			break;
			default:
				printf("Opcion invalida\n");
			break;
		}
	}

	game->closeFlag=1;
	error = SaveData(game->m1, -1);
	error = SaveData(game->m1, -1);
	error = SaveData(game->m1, -2);
	
	// El juego finaliza
	printf("Cerrando local...\n");

	for (int i = 0; i < THREAD; i++)
	{
		pthread_join(th[i], NULL);
	}

	printf("Ordenes tomados: %d\n",game->takenOrders);
	printf("Ordenes cocinados: %d\n",game->cookedOrders);
	printf("Ordenes entregados: %d\n",game->sentOrders);
	printf("Ordenes cobrados: %d\n",game->paidOrders);
	printf("Llamadas perdidas: %d\n",game->lostCalls);
	printf("Dinero total: %d\n",game->totalMoney);
	error = terminate((void *)game);
	if(error) perror("terminate()");
	// fin El juego finaliza

	free(th);	
}

// Funcionamiento básico de la llamada telefónica
void *phone(void *arg){
	struct Game *game = (struct Game *)(arg);
	int state;
	time_t t2;
	srand((unsigned)time(&t2));
	int sleeping = 1;
	while(game->gameTime > 0){
		sleep(sleeping);
		sem_post(game->call);
		sleep(3);
		sem_getvalue(game->call,&state);
		if(state > 0){
			game->lostCalls++;
			sem_wait(game->call);
		}
		sleeping = (rand() % 2) + 1;
	}
	pthread_exit(NULL);
}

// El cocinero recibe una cantidad de pedidos y si corresponde suma, en un momento recibirán el pedido -2 el cual indica que deben dejar de trabajar
void *chef(void *arg){
	struct Game *game = (struct Game *)(arg);
	int error, x;
	int id = game->id;
	int sleeping = 2;

	while (1)
	{
		error = ReadData(game->m1, &x);

		if (error) perror("ReadData()");
		if (x == -1)
		{	
			break;
		}else if(x == -2){
			error = SaveData(game->m2,-1);
			if (error) perror("SaveData()");
			error = SaveData(game->m2,-1);
			if (error) perror("SaveData()");
			break;
		}
		else if(x > 0)
		{
			game->chefState[id] = 1;
			sleep(sleeping);
			error = SaveData(game->m2,x);
			if (error) perror("SaveData()");
			game->cookedOrders++;
			game->chefState[id] = 0;

		}
	}
	pthread_exit(NULL);
}

// El delivery entrega los pedidos del dashboard, y si corresponde incrementa los pedidos.
// y usa la flag de cobro para saber cuando hay una orden a cobrar
// si es -1, entonces se termina de trabajar
void *delivery(void *arg){
	struct Game *game = (struct Game *)(arg);
	int error, x;
	int id = game->id;
	int sleeping=2000;
	while (1)
	{
		error = ReadData(game->m2, &x);
		if (error) perror("ReadData()");
		if (x == -1){
			break;
		}else if(x > 0){
			game->deliveryState[id] = 1;
			usleep(sleeping);
			error= SaveMemory(game->mem,x);
			if(error) perror("SaveMemory)");
			game->payment = 1;
			game->sentOrders++;
			game->deliveryState[id] = 0;
		}
	}
	pthread_exit(NULL);
}

int mainMenu(){
	int exit = 0;
	char ch;
	printf("======================================\n");
	printf("=          TRABAJO PRACTICO 1         =\n");
	printf("=          de Aragon y Godoy         =\n");
	printf("=                                     =\n");
	printf("=            1 - Jugar               =\n");
	printf("=            2 - Cerrar               =\n");
	printf("======================================\n");
	
	do{
		printf("Escoja una opcion: ");
		fflush(stdin);
		ch = getchar();
		switch(ch){
			case 49:
				system("clear");
				printf("Comenzemos...\n");
				return 1;
				break;
			case 50:
				system("clear");
				printf("Cerrando...\n");
				return 2;
				break;
			default:
				printf("Escoja una opcion correcta\n");
				break;
		}
	}while(!exit);
	return 0;
}

// Función que busca el estado del juego
void *GUI(void *arg){
	struct Game *game = (struct Game *)(arg);

	while(game->gameTime > 0){
		if(game->gameTime >= 20) green();
		else if (game->gameTime >= 10 && game->gameTime < 20) yellow();
		else red();
		
		printf("El local cerrara en: %d\n\n",game->gameTime);
		defaultColor();

		sem_getvalue(game->call,&game->phone);
		if(game->phone == 1)
		{
			yellow();
			printf("Llamada entrante...\n");
		}
		else
		{			
			printf("Esperando llamadas\n");
		}

		if(game->payment == 1)
		{
			green();
			printf("Orden lista para cobrar...\n");
		}else{
			defaultColor();
			printf("No hay ordenes a cobrar\n");
		}

		for (int i = 0; i < 3; i++)
		{
			defaultColor();
			printf("Cocinero %d: ",i+1);
			if(game->chefState[i] == 0){
				green();
				printf("Libre\n");
			}else{
				red();
				printf("Cocinando la orden...\n");
			}
		}
		defaultColor();
		
		for (int i = 0; i < 2; i++){
			defaultColor();
			printf("Delivery %d: ",i+1);
			if(game->deliveryState[i] == 0){
				green();
				printf("Libre\n");
			}else{
				red();
				printf("Entregando la orden\n");
			}
		}

		if(game->closeFlag == 1){
			red();
			printf("\nCerrando local\n");
		}else{
			green();
			printf("\nEl local se encuentra abierto\n\n");
		}
		defaultColor();

		printf("1) Atender el telefono\n");
		printf("2) Entregar orden a un chef libre\n");
		printf("3) Cobrar una orden\n");
		printf("Escoja una opcion: \n");
		sleep(1);
		if(game->gameTime > 0){
			system("clear");
		}
	}
	pthread_exit(NULL);
}


void *countdown(void *arg){
	struct Game *game = (struct Game *)(arg);
	while(game->gameTime > 0){
		game->gameTime--;
		sleep(1);
	}
	pthread_exit(NULL);
}

// Leemos el teclado
void *readKeyboard(void *arg){

	struct Game *game = (struct Game *)(arg);
	int option;
	do{
		fflush(stdin);
		scanf("%d",&option);
		switch(option){
			case 1:
				game->option=1;
				break;
			case 2:
				game->option=2;
				break;
			case 3:
				game->option=3;
				break;
			default:
				printf("Opcion incorrecta\n");
				break;
		}
		option = 0;
	}while(game->gameTime > 0);

	pthread_exit(NULL);
}

// Inicialza las variables
int initialize(void *arg )
{
	struct Game *game = (struct Game *)(arg);
	int error = 0;
	game->m1 = NULL;
	game->m2 = NULL;
	game->gameTime = 30;
	game->option = 0;
	game->id = 0;
	game->totalMoney = 0;
	game->phone=0;
	game->closeFlag=0;
	game->payment=0;
	game->takenOrders= 0;
	game->lostCalls = 0;
	game->cookedOrders = 0;
	game->sentOrders = 0;
	game->paidOrders = 0;

	for (int i = 0; i < 3; i++)
	{
		game->chefState[i] = 0;
		if(i != 2){
			game->deliveryState[i] = 0;
		}
	}

	game->call = sem_open("/semBinario2", O_CREAT, O_RDWR, 0);

	if (game->call == SEM_FAILED)
	{
		perror("sem_open()");
		error = -1;
	}

	game->m1 = CreateMonitor();
	if (game->m1 != NULL){
		//perror(""); 
 	}
	else
	{
		perror("CreateMonitor()");
		error = -1;
	}

	game->m2 = CreateMonitor();

	if (game->m2 != NULL){ 
		//perror(""); 
	}
	else
	{
		perror("CreateMonitor()");
		error = -1;
	}
	game->mem = createMemory();
	if (game->mem != NULL){ 
		// 
	}
	else
	{
		perror("createMemory()");
		error = -1;
	}
	return error;
}

// Limpia y finaliza el juego en conjunto con las variables
int terminate(void *arg){
	struct Game *game = (struct Game *)(arg);
	int error = 0;
	if (!error){
		error = sem_close(game->call);
		if (error){
			perror("sem_close()");
		}
	}

	if (!error){
		error = sem_unlink("/semBinario2");
		if (error)
		{
			perror("sem_unlink()");
		}
	}

	delMemory(game->mem);
	DeleteMonitor(game->m1);
	DeleteMonitor(game->m2);
	free(game);
	return error;
}

