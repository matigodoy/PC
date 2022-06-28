#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>
#include <signal.h>


#define TAMMSG 8192
#define tiempoJuego 10

int x = 0;

struct Juego{
	
	//Encargado a Cocinero
	mqd_t msg;
	//Cocinero a Delivery
	mqd_t msg2;
	
	//Semaforo's normales
	sem_t *semtiempoDeJuego;
	sem_t *telefono;
	sem_t *delivery;

	//Semaforos y valores Informativos
	sem_t *pedidosCocinados;
	sem_t *pedidosEntregados;
	sem_t *pedidosCobrados;
	int cierre;
	int opcion;
	
	//pipeline
	int fdTelefono[2];
	///Delivery a Encargado
	int fdFifo;
	//Contador de llamadas perdidas
	int llamadasPerdidas;
};

//Funciones
int Inicializar(struct Juego *game)
{
	int error = 0;
	game->semtiempoDeJuego = sem_open("/semTiempo", O_CREAT, O_RDWR, tiempoJuego);
	game->llamadasPerdidas=0;
	game->cierre = 0;
	if (game->semtiempoDeJuego == SEM_FAILED)
	{
		perror("sem_open()");
		error = -1;
	}
	game->telefono = sem_open("/semTelefono", O_CREAT, O_RDWR, 0);
	if (game->telefono == SEM_FAILED)
	{
		perror("sem_open()");
		error = -1;
	}
	game->delivery = sem_open("/semDelivery", O_CREAT, O_RDWR, 0);
	if (game->delivery == SEM_FAILED)
	{
		perror("sem_open()");
		error = -1;
	}

	game->pedidosCocinados = sem_open("/semCocinados", O_CREAT, O_RDWR, 0);
	if (game->pedidosCocinados == SEM_FAILED)
	{
		perror("sem_open()");
		error = -1;
	}

	game->pedidosEntregados = sem_open("/semEntregados", O_CREAT, O_RDWR, 0);
	if (game->pedidosEntregados == SEM_FAILED)
	{
		perror("sem_open()");
		error = -1;
	}

	game->pedidosCobrados = sem_open("/semCobrados", O_CREAT, O_RDWR, 0);
	if (game->pedidosCobrados == SEM_FAILED)
	{
		perror("sem_open()");
		error = -1;
	}

	game->msg = mq_open("/msg", O_RDWR | O_CREAT, 0666, NULL);
	game->msg2 = mq_open("/msg2", O_RDWR | O_CREAT, 0666, NULL);


	error= mkfifo("./fifoDelivery",0666);
	if ((error<0) && (errno!=EEXIST)) {
      perror("mkfifo");
	}else{
		error=0;
	}
	return error;
}

int Borrar(struct Juego *game)
{
	int error = 0;

	error = sem_close(game->semtiempoDeJuego);
	error = sem_unlink("/semTiempo");

	error = sem_close(game->telefono);
	error = sem_unlink("/semTelefono");

	error = sem_close(game->delivery);
	error = sem_unlink("/semDelivery");

	error = sem_close(game->pedidosCobrados);
	error = sem_unlink("/semCobrados");

	error = sem_close(game->pedidosCocinados);
	error = sem_unlink("/semCocinados");

	error = sem_close(game->pedidosEntregados);
	error = sem_unlink("/semEntregados");

	//Cerrar cola de mensaje
	game->msg = mq_close(game->msg);
	if (game->msg)
	{
		perror("mq_close");
		error = game->msg;
	}
	game->msg = mq_unlink("/msg");
	if (game->msg)
	{
		perror("mq_close");
		error = game->msg;
	}


	game->msg2 = mq_close(game->msg2);
	if (game->msg2)
	{
		perror("mq_close");
		error = game->msg2;
	}
	game->msg2 = mq_unlink("/msg2");
	if (game->msg2)
	{
		perror("mq_close");
		error = game->msg2;
	}


	error = unlink("./fifoDelivery");

	return error;
}

int menu()
{
	int exit = 0;
	char ch;
	printf("1 - Jugar\n");
	printf("2 - Salir\n");
	
	do{
		printf("Ingrese la opcion: ");
		fflush(stdin);
		ch = getchar();
		switch(ch){
			case 49:
				system("clear");
				printf("INICIANDO...\n");
				return 1;
				break;
			case 50:
				return 2;
				break;
			default:
				printf("Dato Incorrecto!\n");
				break;
		}
	}while(!exit);
	return 0;
}

//Threads

void *HiloCocinero(void *arg)
{
	struct Juego *game = (struct Juego *)(arg);
	int recibido;
	char mensaje[TAMMSG];
	while(1){
		recibido=mq_receive(game->msg,mensaje,TAMMSG,NULL);
		if(recibido == -1){
			perror("Error en recibir mensaje");
		}else{
			if(strcmp(mensaje,"-2") == 0){
				mq_send(game->msg2,"-1",3,0);
				mq_send(game->msg2,"-1",3,0);
				break;
			}
			if(strcmp(mensaje,"-1") == 0){
				break;
			}else{
				sleep(2);
				sem_post(game->pedidosCocinados);
				mq_send(game->msg2,mensaje,3,0);
			}
		}
	}
	pthread_exit(NULL);
}

void *HiloDelivery(void *arg)
{
	struct Juego *game = (struct Juego *)(arg);
	int recibido;
	char mensaje[TAMMSG];
	game->fdFifo = open("./fifoDelivery",O_WRONLY);
	while(1){
		recibido=mq_receive(game->msg2,mensaje,TAMMSG,NULL);
		if(recibido == -1){
			perror("Error en recibir mensaje");
		}else{
			if(strcmp(mensaje,"-1") == 0){
				break;
			}else{
				sleep(2);
				if(game->fdFifo < 0){
					perror("While opening Fifo");
				}else{
					sem_post(game->delivery);
					sem_post(game->pedidosEntregados);
					write(game->fdFifo,mensaje,strlen(mensaje));
					
				}
			}
		}
	}
	close(game->fdFifo);
	pthread_exit(NULL);
}

void *timer(void *arg)
{
	struct Juego *game = (struct Juego *)(arg);
	int tiempo;
	sem_getvalue(game->semtiempoDeJuego,&tiempo);
	while(tiempo > 0){
		sem_wait(game->semtiempoDeJuego);
		sem_getvalue(game->semtiempoDeJuego,&tiempo);
		sleep(1);
	}
	pthread_exit(NULL);
}

void *interfazJuego(void *arg)
{
	struct Juego *game = (struct Juego *)(arg);

	int pedidosCobrados,pedidosCocinados,pedidosEntregados,tiempoDeJuego,estadoTelefono,estadoDelivery;

	sem_getvalue(game->semtiempoDeJuego,&tiempoDeJuego);
	while(tiempoDeJuego > 0){
		
		printf("\033[0;31m");
		printf("Tiempo de Juego restante: %d\n\n",tiempoDeJuego);
		printf("\033[0m");
		sem_getvalue(game->telefono,&estadoTelefono);
		if(estadoTelefono % 2 == 1){
			printf("\033[0;33m");
			printf("Llamada entrante\n");
			printf("\033[0m");
		}else{
			printf("\033[0m");
			printf("Telefono apagado\n");
		}

		sem_getvalue(game->delivery,&estadoDelivery);
		if(estadoDelivery >= 1){
			printf("\033[0;33m");
			printf("Delivery en puerta\n");
			printf("\033[0m");
		}else{
			printf("\033[0m");
			printf("nada que cobrar\n");
		}

		if(tiempoDeJuego < 5){
			printf("\033[0;31m");
			printf("\nTERMINANDO...\n");
			printf("\033[0m");
		}else{
			printf("\nDelivery activo\n\n");
		}

		printf("1 - Contestar telefono\n");
		printf("2 - Activar cocinero\n");
		printf("3 - Cobrar delivery\n");
		printf("Ingrese la opcion: \n");
		sleep(1);

		sem_getvalue(game->semtiempoDeJuego,&tiempoDeJuego);
		if(tiempoDeJuego > 0){
			system("clear");
		}
	}
	pthread_exit(NULL);
}

void *leerUsuario(void *arg)
{

	struct Juego *game = (struct Juego *)(arg);
	int opcion,tiempoDeJuego;
	sem_getvalue(game->semtiempoDeJuego,&tiempoDeJuego);
	do{
		fflush(stdin);
		scanf("%d",&opcion);
		switch(opcion){
			case 1:
				game->opcion=1;
				break;
			case 2:
				game->opcion=2;
				break;
			case 3:
				game->opcion=3;
				break;
			default:
				printf("Dato Incorrecto!\n");
				break;
		}
		opcion = 0;
		sem_getvalue(game->semtiempoDeJuego,&tiempoDeJuego);
	}while(tiempoDeJuego > 0);

	pthread_exit(NULL);
}

//Procesos

int Telefono(struct Juego *game)
{
	time_t t;
	srand((unsigned)time(&t));
	int estado,error = 0, esperaAleatoria = 1,tiempo ,aleatorio;
	char texto[3];
	sem_getvalue(game->semtiempoDeJuego,&tiempo);
	while(tiempo > 0){
		usleep(esperaAleatoria);
		esperaAleatoria = ((rand() % 2) + 1) * 50000;
		
		aleatorio = (rand() % 5) + 1;

		snprintf(texto,3,"%d",aleatorio);
		
		write(game->fdTelefono[1],texto,strlen(texto)+1);
		sem_post(game->telefono);
		
		sleep(3);
		//Implementar semaforo telefono

		sem_getvalue(game->telefono,&estado);
		if(estado > 0){
			sem_post(game->telefono);
		}

		sem_getvalue(game->semtiempoDeJuego,&tiempo);
		if(tiempo <= 0){
			//Enviar senal ipc para cerrar todo
			sem_post(game->telefono);
			write(game->fdTelefono[1],"-1",3);
			break;
		}
	}
	return error;
}

int Cocinero(struct Juego *game)
{
	int recibido = 0;
	char mensaje[TAMMSG];

	pthread_t *th2;
	th2 = (pthread_t *)(calloc(2, sizeof(pthread_t)));;

	pthread_create(&th2[0],NULL,HiloCocinero,(void *)(game));
	pthread_create(&th2[1],NULL,HiloCocinero,(void *)(game));

	while(1){
		recibido = mq_receive(game->msg,mensaje,TAMMSG,NULL);
		if(recibido == -1){
			perror("Error en recibir mensaje");
		}else{
			if(strcmp(mensaje,"-2") == 0){
				mq_send(game->msg2,"-1",3,0);
				mq_send(game->msg2,"-1",3,0);
				break;
			}else if(strcmp(mensaje,"-1") == 0){
				break;
			}else{
				sleep(2);
				sem_post(game->pedidosCocinados);
				mq_send(game->msg2,mensaje,3,0);
			}
		}
	}
	for (int i = 0; i < 2; i++)
	{
		pthread_join(th2[i], NULL);
	}
	return 0;
}

int Delivery(struct Juego *game)
{
	int recibido = 0;
	char mensaje[TAMMSG];

	pthread_t *th3;
	th3 = (pthread_t *)(calloc(1, sizeof(pthread_t)));;

	game->fdFifo = open("./fifoDelivery",O_WRONLY);
	pthread_create(&th3[0],NULL,HiloDelivery,(void *)(game));

	while(1){
		recibido=mq_receive(game->msg2,mensaje,TAMMSG,NULL);
		if(recibido == -1){
			perror("Error en recibir mensaje");
		}else{
			if(strcmp(mensaje,"-1") == 0){
				break;
			}else{
				sleep(2);
				if(game->fdFifo < 0){
					perror("While opening Fifo");
				}else{
					sem_post(game->delivery);
					sem_post(game->pedidosEntregados);
					write(game->fdFifo,mensaje,strlen(mensaje)); 
				}
			}
		}
	}
	close(game->fdFifo);
	pthread_join(th3[0], NULL);
	return 0;
}



//Main
int main(int argc, char *argv[])
{
	system("clear");
	if(menu() == 2){
		return 0;
	}
	int pid1, pid2, pid3,error = 0;
	struct Juego *game = NULL;
	game = (struct Juego *)(calloc(1, sizeof(struct Juego)));
	Inicializar(game);
	error = pipe(game->fdTelefono);
	pid1 = fork();
	if (pid1 == 0)
	{
		Telefono(game);
		return 0;
	}
	pid2 = fork();
	if (pid2 == 0)
	{
		Cocinero(game);
		return 0;
	}
	pid3 = fork();
	if (pid3 == 0)
	{
		Delivery(game);
		return 0;
	}

	if (pid1 != 0 && pid2 != 0 && pid3 != 0)
	{
		int sent=0,cantidad,estado = 0,estado2 = 0,entregado = 0,tiempoDeJuego;
		char *remaining;
		char texto[3];

		pthread_t *th;
		th = (pthread_t *)(calloc(1, sizeof(pthread_t)));
		
		time_t t;
		srand((unsigned)time(&t));
		
		pthread_create(&th[0],NULL,timer,(void *)(game));
		pthread_create(&th[1],NULL,interfazJuego,(void *)(game));
		pthread_create(&th[2],NULL,leerUsuario,(void *)(game));
		

		game->fdFifo=open("./fifoDelivery",O_RDONLY);
		sem_getvalue(game->semtiempoDeJuego,&tiempoDeJuego);
		while(tiempoDeJuego > 0){
			if(game->opcion == 1){
				sem_getvalue(game->telefono,&estado);
				if(estado == 1){
					//Atender telefono
					sem_wait(game->telefono);
					cantidad = read(game->fdTelefono[0],texto,3);
				}else if(estado > 1){
					game->llamadasPerdidas++;
					sem_wait(game->telefono);
					sem_wait(game->telefono);
				}
			}else if(game->opcion == 2){
				//Mandar a cocinar
				if(cantidad > 0){
					sent = mq_send(game->msg,texto,3,0);
					cantidad = 0;
					if(sent == -1){
						perror("Error en enviar mensaje");
						error = sent;
					}else{
					}
				}
			}else if(game->opcion == 3){
				sem_getvalue(game->delivery,&estado2);
				if(estado2 >= 1){
					cantidad=read(game->fdFifo,texto,3);
					// String to long int
					entregado = strtol(texto,&remaining,10);
					sem_post(game->pedidosCobrados);
					sem_wait(game->delivery);
				}
			}
			sem_getvalue(game->semtiempoDeJuego,&tiempoDeJuego);
		}
		estado = 1;
		while(estado != 0){
			sem_getvalue(game->telefono,&estado);
			if(estado == 1){
				//Atender telefono
				sem_wait(game->telefono);
				cantidad = read(game->fdTelefono[0],texto,3);
			}else if(estado > 1){
				game->llamadasPerdidas++;
				sem_wait(game->telefono);
				sem_wait(game->telefono);
			}
		}

		close(game->fdFifo);

		sent = mq_send(game->msg,"-1",3,0);
		sent = mq_send(game->msg,"-1",3,0);
		sent = mq_send(game->msg,"-2",3,0);

		game->cierre = 1;
		while(wait(NULL) != -1 || errno != ECHILD){
		}
		system("clear");
		printf("\033[0;33m");
		fprintf(stdout,"GAME OVER!\n");
		printf("\033[0m");

		int pedidosCobrados,pedidosCocinados,pedidosEntregados;

		sem_getvalue(game->pedidosCobrados,&pedidosCobrados);
		sem_getvalue(game->pedidosCocinados,&pedidosCocinados);
		sem_getvalue(game->pedidosEntregados,&pedidosEntregados);

		fprintf(stdout,"Llamadas Perdidas: %d\n",game->llamadasPerdidas);
		fprintf(stdout,"Pedidos Cocinados: %d\n",pedidosCocinados);
		fprintf(stdout,"Pedidos Entregados: %d\n",pedidosEntregados);
		fprintf(stdout,"Pedidos Cobrados: %d\n",pedidosCobrados);

		free(th);
		Borrar(game);
	}

	return 0;
}












