
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define MAX_THREADS 9

int P;
int *arreglo;
int max;

void *buscar(void *arg){
    int *id = (int *)arg;
    int inicio = (*id)*(P/MAX_THREADS);
    int fin = inicio + (P/MAX_THREADS);
    int i;
    for(i=inicio;i<fin;i++){
        if(arreglo[i]>max){
            max = arreglo[i];
        }
    }
    pthread_exit(NULL);
}

int main(){
    int i;
    int *id;
    pthread_t threads[MAX_THREADS];
    printf("Ingrese el valor de P: ");
    scanf("%d",&P);
    arreglo = (int *)malloc(sizeof(int)*P);
    srand
    for(i=0;i<P;i++){
        arreglo[i] = rand()%P;
    }
    arreglo[rand()%P] = P;
    max = 0;
    for(i=0;i<MAX_THREADS;i++){
        id = (int *)malloc(sizeof(int));
        *id = i;
        pthread_create(&threads[i],NULL,buscar,(void *)id);
    }
    for(i=0;i<MAX_THREADS;i++){
        pthread_join(threads[i],NULL);
    }
    printf("El valor maximo es: %d\n",max);
    return 0;
}