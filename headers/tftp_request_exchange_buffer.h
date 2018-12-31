// must be included after tftp_messages.h
#ifndef TFTP_REQUEST_EXCHANGE_BUFFER_H
#define TFTP_REQUEST_EXCHANGE_BUFFER_H

#include <pthread.h>

// include tftp_messages.h first

struct tftp_request_exchange_buffer {
  pthread_mutex_t M;
  //pthread_cond_t FULL; // the producer will not block in a condition wait
  pthread_cond_t EMPTY;

  int head, tail, size;
  struct tftp_message* *requests;
  // use head to deposit
  // use tail to withdraw

  struct sockaddr_in* *clients;
};

typedef tftp_reb struct tftp_request_exchange_buffer;

int tftp_request_exchange_buffer_init(tftp_reb * reb, int size);
void tftp_request_withdraw(tftp_reb * reb, struct tftp_message* *message_ref,
			   struct sockaddr_in* *client);
int tftp_request_deposit(tftp_reb * reb, struct tftp_message * message_ref,
			 struct sockaddr_in *client);

#define TFTP_REB_INIT_SUCCESS 0
#define TFTP_REB_INIT_INVALID_SIZE 1

#define TFTP_REB_DEPOSIT_SUCCESS 0
#define TFTP_REB_DEPOSIT_FULL 1


#endif
