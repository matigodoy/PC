#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct pedido
{
    int numero;
    int tiempo_preparacion;
    int tiempo_delivery;
} pedido_t;

typedef struct cocinero
{
    int numero;
    int tiempo_preparacion;
    int tiempo_delivery;
} cocinero_t;

typedef struct delivery
{
    int numero;
    int tiempo_preparacion;
    int tiempo_delivery;
} delivery_t;

typedef struct encargado
{
    int numero;
    int tiempo_preparacion;
    int tiempo_delivery;
} encargado_t;

typedef struct telefono
{
    int numero;
    int tiempo_preparacion;
    int tiempo_delivery;
} telefono_t;

sem_t sem_cocinero;
sem_t sem_delivery;
sem_t sem_encargado;
sem_t sem_telefono;

int pedidos_atendidos = 0;
int pedidos_cocinados = 0;
int pedidos_entregados = 0;
int limite_pedidos = 3;


void *cocinero(void *arg);
void *delivery(void *arg);
void *encargado(void *arg);
void *telefono(void *arg);

int main(int argc, char *argv[])
{
    pthread_t cocinero1, cocinero2, cocinero3, delivery1, delivery2, encargado1, telefono1;

    cocinero_t cocinero_1 = {1, 1, 1};
    cocinero_t cocinero_2 = {2, 2, 2};
    cocinero_t cocinero_3 = {3, 3, 3};

    delivery_t delivery_1 = {1, 1, 1};
    delivery_t delivery_2 = {2, 2, 2};

    encargado_t encargado_1 = {1, 1, 1};

    telefono_t telefono_1 = {1, 1, 1};

    sem_init(&sem_cocinero, 0, 0);
    sem_init(&sem_delivery, 0, 0);
    sem_init(&sem_encargado, 0, 0);
    sem_init(&sem_telefono, 0, 1);

    pthread_create(&cocinero1, NULL, cocinero, &cocinero_1);
    pthread_create(&cocinero2, NULL, cocinero, &cocinero_2);
    pthread_create(&cocinero3, NULL, cocinero, &cocinero_3);
    pthread_create(&delivery1, NULL, delivery, &delivery_1);
    pthread_create(&delivery2, NULL, delivery, &delivery_2);
    pthread_create(&encargado1, NULL, encargado, &encargado_1);
    pthread_create(&telefono1, NULL, telefono, &telefono_1);

    pthread_join(cocinero1, NULL);
    pthread_join(cocinero2, NULL);
    pthread_join(cocinero3, NULL);
    pthread_join(delivery1, NULL);
    pthread_join(delivery2, NULL);
    pthread_join(encargado1, NULL);
    pthread_join(telefono1, NULL);

    sem_destroy(&sem_cocinero);
    sem_destroy(&sem_delivery);
    sem_destroy(&sem_encargado);
    sem_destroy(&sem_telefono);

    return 0;
}


void *cocinero(void *arg)
{
    cocinero_t *cocinero = (cocinero_t *)arg;
    while (pedidos_entregados<limite_pedidos)
    {
        sem_wait(&sem_cocinero);
        pedidos_cocinados++;
        printf("El cocinero %d esta preparando el pedido %d \n", cocinero->numero, pedidos_cocinados);
        sleep(cocinero->tiempo_preparacion);
        printf("El cocinero %d termino de preparar el pedido %d\n", cocinero->numero, pedidos_cocinados);
        sem_post(&sem_delivery);
    }
    pthread_exit(NULL);
}

void *delivery(void *arg)
{
    delivery_t *delivery = (delivery_t *)arg;
    while (pedidos_entregados<limite_pedidos)
    {
        sem_wait(&sem_delivery);
        pedidos_entregados++;
        printf("El delivery %d esta entregando el pedido %d\n", delivery->numero, pedidos_entregados);
        sleep(delivery->tiempo_delivery);
        printf("El delivery %d termino de entregar el pedido %d\n", delivery->numero, pedidos_entregados);
        sem_post(&sem_encargado);
    }
    pthread_exit(NULL);
}

void *encargado(void *arg)
{
    encargado_t *encargado = (encargado_t *)arg;
    while (pedidos_entregados<limite_pedidos)
    {
        sem_wait(&sem_encargado);
        printf("El encargado %d esta cobrando el pedido %d\n", encargado->numero, pedidos_entregados);
        sleep(encargado->tiempo_preparacion);
        printf("El encargado %d termino de cobrar el pedido %d\n", encargado->numero, pedidos_entregados);
        sem_post(&sem_telefono);
        if (pedidos_entregados>=limite_pedidos)
        {
            exit(EXIT_SUCCESS);
        }
    }
    pthread_exit(NULL);
}

void *telefono(void *arg)
{
    telefono_t *telefono = (telefono_t *)arg;
    while (pedidos_entregados<limite_pedidos)
    {
        sem_wait(&sem_telefono);
        pedidos_atendidos++;
        printf("El telefono %d esta recibiendo el pedido %d\n", telefono->numero, pedidos_atendidos);
        sleep(telefono->tiempo_delivery);
        printf("El telefono %d termino de recibir el pedido %d\n", telefono->numero, pedidos_atendidos);
        sem_post(&sem_cocinero);
    }
    pthread_exit(NULL);
}