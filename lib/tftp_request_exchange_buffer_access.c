#include "../headers/tftp_messages.h"
#include "../headers/tftp_request_buffer_exchange.h"

#include <stdlib.h>
#include <pthread.h>


void tftp_request_withdraw(tftp_reb * reb, struct tftp_message* *message_ref){

  // get lock on structure
  pthread_mutex_lock(&reb->M);
  while(reb->head == reb->tail)
    // wait for requests
    pthread_cond_wait(&reb->EMPTY, &reb->M);

  // provide the caller with the memory location
  // where the tftp_request has been allocated
  *message_ref = reb->requests[reb->tail++];
  reb->tail %= reb->size;

  pthread_mutex_unlock(&reb->M);
}


int tftp_request_deposit(tftp_reb * reb, struct tftp_message * message_ref){

  // get lock
  pthread_mutex_lock(&reb->M);
  if(((reb->tail + 1 ) % reb->size) == reb->head){
    pthread_mutex_unlock(&reb->M);
    return TFTP_REB_DEPOSIT_FULL;
  }

  reb->requests[reb->head] = message_ref;

  if(reb->head == reb->tail)
    pthread_cond_broadcast(&reb->EMPTY);
  
  reb->head = ++reb->head % reb->size;

  pthread_mutex_unlock(&reb->M);

  return TFTP_REB_DEPOSIT_SUCCESS;
}
