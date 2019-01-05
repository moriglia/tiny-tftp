#ifndef SAFEPRINT_H
#define SAFEPRINT_H

#include <pthread.h>

void safe_printf(pthread_mutex_t * M, const char * format, ...);
void safe_perror(pthread_mutex_t * M, const char * format, ...);

#endif
