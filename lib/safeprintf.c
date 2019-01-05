#include "../headers/safeprintf.h"
#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>


void safe_printf(pthread_mutex_t * M, const char * format, ...){
  va_list args;
  va_start(args, format);

  pthread_mutex_lock(M);
  vprintf(format, args);
  pthread_mutex_unlock(M);

  va_end(args);

  return ;
}


void safe_perror(pthread_mutex_t * M, const char * format, ...){
  va_list args;
  char error_string[100];
  
  va_start(args, format);
  vsprintf(error_string, format, args);
  va_end(args);

  pthread_mutex_lock(M);
  perror(error_string);
  pthread_mutex_unlock(M);
  
  return ;
}
