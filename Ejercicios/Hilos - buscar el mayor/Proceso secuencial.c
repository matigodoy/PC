#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define MAX_THREADS 9

int *array;
int max;
int size;

void *max_search(void *arg)
{
    int *data = (int *)arg;
    int max_local = 0;
    int i;

    for (i = data[0]; i < data[1]; i++)
    {
        if (array[i] > max_local)
        {
            max_local = array[i];
        }
    }

    if (max_local > max)
    {
        max = max_local;
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int i;
    int max_position;
    int *data;
    pthread_t threads[MAX_THREADS];

    srand(time(NULL));

    printf("Ingrese el tamaño del arreglo: ");
    scanf("%d", &size);

    array = (int *)malloc(sizeof(int) * size);

    for (i = 0; i < size; i++)
    {
        array[i] = rand() % size;
    }

    max_position = rand() % size;
    array[max_position] = size;

    max = 0;

    data = (int *)malloc(sizeof(int) * 2);
    data[0] = 0;
    data[1] = size / MAX_THREADS;

    for (i = 0; i < MAX_THREADS; i++)
    {
        pthread_create(&threads[i], NULL, max_search, (void *)data);
        data[0] = data[1];
        data[1] += size / MAX_THREADS;
    }

    for (i = 0; i < MAX_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    printf("El valor máximo es: %d\n", max);

    free(array);
    free(data);

    return 0;
}