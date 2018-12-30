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
};

typedef tftp_reb struct tftp_request_exchange_buffer;

void tftp_request_withdraw(tftp_reb * reb, struct tftp_message* *message_ref);
int tftp_request_deposit(tftp_reb * reb, struct tftp_message* *message_ref);

#define TFTP_REB_DEPOSIT_SUCCESS 0
#define TFTP_REB_DEPOSIT_FULL 1


#endif
