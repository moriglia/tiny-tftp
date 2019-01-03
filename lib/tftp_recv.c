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

int tftp_recv(int sockfd, struct tftp_message* const message,
	      struct sockaddr_in* src_addr, socklen_t *addrlen){
  int ret;
  void *buffer = malloc(MAX_BUFFER_SIZE);
  int buffersize = MAX_BUFFER_SIZE;

  ret = recvfrom(sockfd, buffer, buffersize, 0,
		 (struct sockaddr*)src_addr, addrlen);
  if(ret<0)
    return ret;

  return unpack_message(buffer, message, ret);
}
