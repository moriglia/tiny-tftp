#include "../headers/tftp_messages.h"
#include "../headers/tftp_request_exchange_buffer.h"

#include <stdlib.h>
#include <pthread.h>


int tftp_request_exchange_buffer_init(tftp_reb * reb, int size){
  if (size < 0)
    return TFTP_REB_INIT_INVALID_SIZE;

  reb->requests = malloc(size * sizeof(struct tftp_message *));
  if(!reb->requests)
    return TFTP_REB_INIT_MEMORY_LACK;

  reb->clients = malloc(size * sizeof(struct sockaddr_in * ));
  if (!reb->clients){
    free(reb->requests);
    return TFTP_REB_INIT_MEMORY_LACK;
  }
  
  reb->size = size;
  reb->head = 0;
  reb->tail = 0;

  pthread_mutex_init(&reb->M, NULL);
  pthread_cond_init(&reb->EMPTY, NULL);

  return TFTP_REB_INIT_SUCCESS;
}

void tftp_request_withdraw(tftp_reb * reb, struct tftp_message* *message_ref,
			   struct sockaddr_in* *client){

  // get lock on structure
  pthread_mutex_lock(&reb->M);
  while(reb->head == reb->tail)
    // wait for requests
    pthread_cond_wait(&reb->EMPTY, &reb->M);

  // provide the caller with the memory location
  // where the tftp_request has been allocated
  *message_ref = reb->requests[reb->tail];

  // provide the coller with the memory location
  // where the client address has been allocated
  *client = reb->requests[reb->tail++];

  reb->tail %= reb->size;

  pthread_mutex_unlock(&reb->M);
}


int tftp_request_deposit(tftp_reb * reb, struct tftp_message * message_ref,
			 struct sockaddr_in *client){

  // get lock
  pthread_mutex_lock(&reb->M);
  if(((reb->tail + 1 ) % reb->size) == reb->head){
    pthread_mutex_unlock(&reb->M);
    return TFTP_REB_DEPOSIT_FULL;
  }

  reb->requests[reb->head] = message_ref;
  reb->clients[reb->head] = client;

  if(reb->head == reb->tail)
    pthread_cond_broadcast(&reb->EMPTY);
  
  reb->head = (reb->head + 1) % reb->size;

  pthread_mutex_unlock(&reb->M);

  return TFTP_REB_DEPOSIT_SUCCESS;
}
