

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "shared.h"

struct Memory *createMemory()
{
	int error=0;
	struct Memory *aux = NULL;
	aux = (struct Memory *)(calloc(1, sizeof(struct Memory)));
	aux->memory = 0;
	aux->binary = sem_open("/semBinario", O_CREAT, O_RDWR, 0);
	if (aux->binary == SEM_FAILED)
	{
		perror("sem_open()");
		error = -1;
	}
	if (!error)
	{
		aux->memory = shm_open("/memCompartida", O_CREAT | O_RDWR, 0660);
		if (aux->memory < 0)
		{
			perror("shm_open()");
			error = -1;
		}
	}
	if (!error)
	{
		error = ftruncate(aux->memory, sizeof(int));
		if (error) perror("ftruncate()");
	}
	return aux;
}

int SaveMemory(struct Memory *m, int dato)
{
	int error=0;
	int *datos = NULL;
	datos = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, m->memory, 0);

	if (datos == MAP_FAILED)
	{
		perror("mmap()");
		error = -1;
  	}
	if (!error)
	{
		error = sem_wait(m->binary);
		if (error)
		{
			perror("sem_wait()");
		}
		if (!error)
		{
			*datos = dato;
			usleep(random() % 5000000);
			error = sem_post(m->binary);
			if (error)
			{
				perror("sem_post()");
			}
		}
	}
	if (datos != NULL)
	{
		error = munmap((void *)(datos), 2 * sizeof(int));
		if (error)
		{
			perror("munmap()");
		}
	}
	return error;
}


int readMemory(struct Memory *m, int *dato)
{
	int error= 0;
	int *mapeo = NULL;

	mapeo = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, m->memory, 0);

	if (mapeo == MAP_FAILED)
	{
		perror("mmap()");
		error = -1;
	}

	if (!error)
	{
		error = sem_post(m->binary);
		if (error)
			perror("sem_post()");
	}

	fflush(NULL);
	*dato = *mapeo;
	*mapeo = 0;

	if (mapeo != NULL)
	{
		error = munmap((void *)(mapeo), 2 * sizeof(int));
		if (error)
		{
			perror("munmap()");
		}
	}

	return error;
}

void delMemory(struct Memory *m)
{
	int error=0;
	if(m != NULL)
	{
		if (!error)
		{
			error = sem_close(m->binary);
			if (error)
			{
				perror("sem_close()");
			}
		}

		if (!error)
		{
			error = sem_unlink("/semBinario");
			if (error)
			{
				perror("sem_unlink()");
			}
		}

		if (m->memory > 0)
		{
			error = shm_unlink("/memCompartida");
			if (error)
			{
				perror("unlink()");
			}
		}
	}
}
