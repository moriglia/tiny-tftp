#include "../headers/tftp_messages.h"
#include "../headers/tftp_constants.h"
#include "../headers/pack_message.h"
#include "../headers/tftp_request_exchange_buffer.h"
#include "../headers/tftp_message_alloc.h"

// network library headers
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

// other utility headers
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>


#define MAX_BUFFER_SIZE 2048

int tftp_send(int sockfd, const struct tftp_message* const message,
	      struct sockaddr* dest_addr, socklen_t addrlen){
  void buffer[MAX_BUFFER_SIZE];
  int buffersize = MAX_BUFFER_SIZE;
  
  ret = pack_message(message, buffer, &buffersize);
  if (ret != PACK_SUCCESS){
    errno = EINVAL;
    return -1;
  }

  return sendto(sockfd, buffer, (size_t)buffersize, 0, dest_addr, addrlen);
}
x
