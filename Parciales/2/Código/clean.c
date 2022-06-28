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
#include <mqueue.h>

int main(int argc, char *v[]){

	sem_unlink("/semTiempo");
	sem_unlink("/semTelefono");
	sem_unlink("/semDelivery");
	mq_unlink("/mensajes");
	mq_unlink("/mensajes2");
	unlink("/tmp/fifoDelivery");
	unlink("./fifoDelivery");

	printf("Semaforos borrados");
	return 0;
}