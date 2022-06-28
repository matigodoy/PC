#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

#include "buffer.h"

struct Monitor *CreateMonitor()
{
	int error = 0;
	struct Monitor *aux = NULL;

	aux = (struct Monitor *)(calloc(1, sizeof(struct Monitor)));

	if (aux != NULL)
	{
		aux->start = 0;
		aux->end = 0;
		aux->count = 0;
		error += pthread_cond_init(&aux->cond, NULL);
		if (error)
			perror("pthread_cond_init()");

		error += pthread_mutex_init(&aux->read, NULL);
		if (error)
			perror("pthread_mutex_init()");

		error += pthread_mutex_init(&aux->write, NULL);
		if (error)
			perror("pthread_mutex_init()");

		pthread_cond_broadcast(&aux->cond);
		pthread_mutex_lock(&aux->read);
		pthread_mutex_unlock(&aux->write);
	}
	return aux;
}

int SaveData(struct Monitor *m, int data)
{
	int error = 0;

	error = pthread_mutex_lock(&m->write);
	if (error)
		perror("pthread_mutex_lock()");
	else
		// Verificar si el contador esta siendo utilizado para verificar que se este usando como controlador
		while (m->count > 9)
			error = pthread_cond_wait(&m->cond, &m->write);

	if (error)
		perror("pthread_cond_wait()");
	else
	{
		m->data[m->end] = data;
		m->end = ++m->end % 10;
		++m->count;
		pthread_cond_signal(&m->cond);
	}

	if (error)
		perror("pthread_cond_signal()");
	else
		error = pthread_mutex_unlock(&m->read);

	if (error)
		perror("pthread_mutex_unlock()");

	return error;
}

int ReadData(struct Monitor *m, int *data)
{
	int error = 0;

	error = pthread_mutex_lock(&m->read);
	if (error)
		perror("pthread_mutex_lock()");
	else
		while (m->count < 0)
			// Si el contador es menor a 1 significa que no hay elementos a leer
			error = pthread_cond_wait(&m->cond, &m->read);

	if (error)
		perror("pthread_cond_wait()");
	else
	{

		*data = m->data[m->start];
		m->start = ++m->start % 10;
		--m->count;
		pthread_cond_signal(&m->cond);
	}

	if (error)
		perror("pthread_cond_signal()");
	else
		error = pthread_mutex_unlock(&m->write);

	if (error)
		perror("pthread_mutex_unlock()");

	return error;
}

void DeleteMonitor(struct Monitor *m)
{
	if (m != NULL)
	{
		pthread_cond_destroy(&m->cond);
		pthread_mutex_destroy(&m->read);
		pthread_mutex_destroy(&m->write);
		free(m);
	}
}
