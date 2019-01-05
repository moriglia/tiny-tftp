#include "../headers/tftp_messages.h"
#include "../headers/tftp_constants.h"
#include "../headers/tftp_display.h"

#include <stdio.h>
#include <pthread.h>

void safe_tftp_display(pthread_mutex_t * M, struct tftp_message * ptr){
  pthread_mutex_lock(M);
  tftp_display(ptr);
  pthread_mutex_unlock(M);
}
