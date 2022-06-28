#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
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

pthread_mutex_t mutex_cocinero = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_delivery = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_encargado = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_telefono = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cond_cocinero = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_delivery = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_encargado = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_telefono = PTHREAD_COND_INITIALIZER;

int pedidos_totales = 0;
int pedidos_preparados = 0;
int pedidos_entregados = 0;
int pedidos_cobrados = 0;

void *cocinero(void *arg)
{
    cocinero_t *cocinero = (cocinero_t *)arg;
    while (1)
    {
        pthread_mutex_lock(&mutex_cocinero);
        while (pedidos_preparados == pedidos_totales)
        {
            pthread_cond_wait(&cond_cocinero, &mutex_cocinero);
        }
        printf("Cocinero %d preparando pedido %d\n", cocinero->numero, pedidos_preparados + 1);
        sleep(cocinero->tiempo_preparacion);
        printf("Cocinero %d termino de preparar pedido %d\n", cocinero->numero, pedidos_preparados + 1);
        pedidos_preparados++;
        pthread_cond_signal(&cond_delivery);
        pthread_mutex_unlock(&mutex_cocinero);
    }
    pthread_exit(NULL);
}

void *delivery(void *arg)
{
    delivery_t *delivery = (delivery_t *)arg;
    while (1)
    {
        pthread_mutex_lock
        (&mutex_delivery);
        while (pedidos_entregados == pedidos_totales)
        {
            pthread_cond_wait(&cond_delivery, &mutex_delivery);
        }
        printf("Delivery %d entregando pedido %d\n", delivery->numero, pedidos_entregados + 1);
        sleep(delivery->tiempo_delivery);
        printf("Delivery %d termino de entregar pedido %d\n", delivery->numero, pedidos_entregados + 1);
        pedidos_entregados++;
        pthread_cond_signal(&cond_encargado);
        pthread_mutex_unlock(&mutex_delivery);
    }
    pthread_exit(NULL);
}

void *encargado(void *arg)
{
    encargado_t *encargado = (encargado_t *)arg;
    while (1)
    {
        pthread_mutex_lock(&mutex_encargado);
        while (pedidos_cobrados == pedidos_totales)
        {
            pthread_cond_wait(&cond_encargado, &mutex_encargado);
        }
        printf("Encargado %d cobrando pedido %d\n", encargado->numero, pedidos_cobrados + 1);
        sleep(encargado->tiempo_preparacion);
        printf("Encargado %d termino de cobrar pedido %d\n", encargado->numero, pedidos_cobrados + 1);
        pedidos_cobrados++;
        pthread_cond_signal(&cond_telefono);
        pthread_mutex_unlock(&mutex_encargado);
    }
    pthread_exit(NULL);
}

void *telefono(void *arg)
{
    telefono_t *telefono = (telefono_t *)arg;
    while (1)
    {
        pthread_mutex_lock(&mutex_telefono);
        while (pedidos_totales == 10)
        {
            pthread_cond_wait(&cond_telefono, &mutex_telefono);
        }
        printf("Telefono %d recibiendo pedido %d\n", telefono->numero, pedidos_totales + 1);
        sleep(telefono->tiempo_preparacion);
        printf("Telefono %d termino de recibir pedido %d\n", telefono->numero, pedidos_totales + 1);
        pedidos_totales++;
        pthread_cond_signal(&cond_cocinero);
        pthread_mutex_unlock(&mutex_telefono);
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    pthread_t cocinero1, cocinero2, delivery1, delivery2, encargado1, encargado2, telefono1, telefono2;
    cocinero_t cocinero_1 = {1, 1, 1};
    cocinero_t cocinero_2 = {2, 1, 1};
    delivery_t delivery_1 = {1, 1, 1};
    delivery_t delivery_2 = {2, 1, 1};
    encargado_t encargado_1 = {1, 1, 1};
    encargado_t encargado_2 = {2, 1, 1};
    telefono_t telefono_1 = {1, 1, 1};
    telefono_t telefono_2 = {2, 1, 1};

    pthread_create(&cocinero1, NULL, cocinero, &cocinero_1);
    pthread_create(&cocinero2, NULL, cocinero, &cocinero_2);
    pthread_create(&delivery1, NULL, delivery, &delivery_1);
    pthread_create(&delivery2, NULL, delivery, &delivery_2);
    pthread_create(&encargado1, NULL, encargado, &encargado_1);
    pthread_create(&encargado2, NULL, encargado, &encargado_2);
    pthread_create(&telefono1, NULL, telefono, &telefono_1);
    pthread_create(&telefono2, NULL, telefono, &telefono_2);

    pthread_join(cocinero1, NULL);
    pthread_join(cocinero2, NULL);
    pthread_join(delivery1, NULL);
    pthread_join(delivery2, NULL);
    pthread_join(encargado1, NULL);
    pthread_join(encargado2, NULL);
    pthread_join(telefono1, NULL);
    pthread_join(telefono2, NULL);

    return 0;
}