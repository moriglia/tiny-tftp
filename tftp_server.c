#include "headers/tftp_messages.h"
#include "headers/tftp_constants.h"
#include "headers/pack_message.h"
#include "headers/tftp_request_exchange_buffer.h"
#include "headers/tftp_message_alloc.h"

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

#define MAX_BUFFER_SIZE 1024
#define WORKERS_NUMBER 4

#define ILLEGAL_OPERATION_STRING "Illegal operation (note: WRQ not accepted)"
#define ILLEGAL_OPERATION_LEN 33

#define MAX_REB_SIZE 10
tftp_reb r;
/* 
request exchange buffer:
    used to deliver requests to worker threads.
    Used through the functions provided in
        tftp_request_exchange_buffer.h
*/


int main(int argc, char** argv){
  // variables -----------------------------------------
  int ret,   // catch return status of functions
    dir_len, // length of the directory string
    buf_len,
    aux;
  
  int listener_sd;
  struct sockaddr_in listener_address, * client_address;

  int port;
  //char * directory;

  pthread_t workers[MAX_REB_SIZE];

  tftp_message * tftp_msg;
  void buffer[MAX_BUFFER_SIZE];

  ssize_t size;

  // init of illegal operation message

  // ----------------------------------------------------
  if (argc != 3){
    printf("Usage: tftp_server <port> <directory>\n");
    printf("\t<port>\t1 - 65535\n");
    printf("\t<directory>\tDirectory with the files to serve\n");
    exit(255);
  }

  port = atoi(argv[1]);
  if (port > 65535 || port < 1){
    printf("Port out of range: %d\n", port);
    printf("Usage: tftp_server <port> <directory>\n");
    printf("\t<port>\t1 - 65535\n");
    printf("\t<directory>\tDirectory with the files to serve\n");
    exit(255);
  }
  dir_len = strlen(argv[2]);
  

  // reb initialization
  ret = tftp_request_exchange_buffer_init(&r, MAX_REB_SIZE);
  if (ret == TFTP_REB_INIT_INVALID_SIZE){
    printf("Invalid size for the request exchange buffer");
    exit(-1);
  }


  // listener address initialization
  memset(&listener_address, 0, sizeof(struct sockaddr_in));
  listener_address.sin_addr.s_addr = INADDR_ANY;
  listener_address.sin_family = AF_INET;
  listener_address.sin_port = htons(port);

  // socket initialization
  listener_sd = socket(AF_INET, SOCK_DGRAM, 0);
  if (listener_sd == -1){
    perror("Socket creation failed");
    exit(errno);
  }

  // binding
  ret = bind(listener_sd, (struct sockaddr*)listener_address,
	     sizeof(struct sockaddr));
  if (ret == -1) {
    if(errno == EADDRINUSE){
      perror("Binding failed, try another port");
      close(listener_sd);
      exit(errno);
    }
    perror("Binding error, see details");
    exit(errno);
  }

  // worker creation
  for(int i = 0; i < WORKERS_NUMBER; ++i){
    // activate workers TODO
  }



  // handle requests
  client_address = malloc(sizeof(struct sockaddr_in));
  memset(client_address, 0, sizeof(struct sockaddr_in));
  for(;;){
    // wait for requests
    size = recvfrom(listener_sd, buffer, MAX_BUFFER_SIZE,
		    (struct sockaddr *)client_address, sizeof(struct sockaddr_in));
    if(size == -1){
      perror("Failed in receiving the UDP packet");
      // send error ---------- TODO
      continue ;
    }

    // unpack request
    tftp_msg = tftp_message_alloc();
    ret = unpack_request(buffer, tftp_msg, size);
    switch(ret){
    case PACK_SUCCESS:
      // nothing to do
      break;
      
    // to implement further error control just add cases here

    default:
      // in case of error, just continue
      tftp_message_free((void*)tftp_msg);
      continue;
    }

    
    switch(tftp_msg->opcode){
    case OPCODE_RRQ:
      break;
    default:
      // prepare error message
      tftp_msg->opcode = OPCODE_ERROR;
      tftp_msg->error_code = TFTP_ERROR_ILLEGAL;
      tftp_msg->error_message = malloc(ILLEGAL_OPERATION_LEN);
      strncpy(tftp_msg->error_message,
	      ILLEGAL_OPERATION_STRING,
	      ILLEGAL_OPERATION_LEN);

      //pack error message
      buf_len = MAX_BUFFER_SIZE;
      ret = pack_message(tftp_msg, buffer, &buf_len);

      // send error message
      ret = sendto(listener_sd, buffer, buf_len, 0,
		   (struct sockaddr*)client_address, sizeof(struct sockaddr_in));
      // no check is fine.

      // reset client_address, delete tftp_msg and continue
      memset(client_address, 0, sizeof(struct sockaddr_in));
      tftp_message_free((void*)tftp_message);
      continue ;
    }

    aux = 10;
    for (ret = tftp_reb_deposit(&r, tftp_msg, client_address);
	 ret != TFTP_REB_DEPOSIT_SUCCESS && aux > 0; --aux){
      ret = tftp_reb_deposit(&r, tftp_msg, client_address);
      sleep(1);
    }

    if(ret != TFTP_REB_DEPOSIT_SUCCESS){
      printf("Dead workers and buffer full\nExiting\n");
      exit(255);
    }

    client_address = malloc(sizeof(struct sockaddr_in));
    
  }
  
}
