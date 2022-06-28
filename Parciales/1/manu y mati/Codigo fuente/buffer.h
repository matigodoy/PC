#ifndef _MONITORSIMPLE_H_
#define _MONITORSIMPLE_H_

// Definicion de la interfaz de monitor
struct Monitor
{
	int count;
	int data[10];
	int start, end;
	pthread_cond_t cond;
	pthread_mutex_t write, read;
};
struct Monitor *CreateMonitor();
int SaveData(struct Monitor *m, int data);	// Guarda los datos
int ReadData(struct Monitor *m, int *data); // Leer los datos del monitor
void DeleteMonitor(struct Monitor *m);		// Eliminar el monitor

#endif
