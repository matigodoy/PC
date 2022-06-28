#ifndef _MEMORIA_H_

	#define _MEMORIA_H_
	#include <semaphore.h>

	struct Memory{
		int memory;
		sem_t *binary;
	};

	struct Memory *createMemory();

	int SaveMemory(struct Memory *m,int dato);

	int readMemory(struct Memory *m,int *dato);

	void delMemory(struct Memory *m);

#endif