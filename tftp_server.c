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

void * worker_thread(void* arg){
  int sock_fd, ret, aux, socklen;
  struct tftp_message * message_pointer, * reply_pointer;
  struct sockaddr_in * client_pointer, worker_address, *recv_pointer;

  // reading variables
  FILE * file_pointer;
  char buffer[MAX_BUFFER_SIZE];
  

  // address configuration, binding once for each request to serve
  worker_address.sin_family = AF_INET;
  worker_address.sin_addr.s_addr = INADDR_ANY;

  for(;;){
    //1: withdraw request
    tftp_request_withdraw(&r, &message_pointer, &client_pointer);
    
    //2: choose a random port and bind to socket
    do {
      worker_address.sin_port = rand() + 1024;

      // socket creation
      sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
      if (sock_fd < 0){
	perror("Socket creation failed in thread. Exiting thread");
	pthread_exit(255);
      }
  
      ret = bind(sock_fd, (struct sockaddr*)&worker_address,
		 sizeof(struct sockaddr_in));
      
    } while(ret != 0);
    
    //3: open file to send
    switch(message_pointer->mode){
    case MODE_NETASCII:
      file_pointer = fopen(message_pointer->filename, "r");
      break;
    case MODE_OCTET:
      file_pointer = fopen(message_pointer->filename, "rb");
      break;
    case MODE_MAIL:
    default:
      // not supported
      file_pointer = NULL;
    }

    // not necessary any more
    tftp_message_free(message_pointer);

    //4: check for file existence
    if (!file_pointer){
      reply_pointer = tftp_message_alloc();
      if (reply_pointer){
	reply_pointer->opcode = OPCODE_ERROR;
	reply_pointer->error_code = TFTP_ERROR_FILE_NOT_FOUND;
	reply_pointer->error_message = 0;
	ret = tftp_send(sock_fd, reply_pointer, client_pointer,
			sizeof(struct sockaddr_in));
	// do not handle error
	tftp_message_free(reply_pointer);
      }
      free(client_pointer);
      continue; // without sending error
    }


    // reply structure allocation
    reply_pointer = tftp_message_alloc();
    if(!reply_pointer){
      printf("Allocation of reply packet failed: exiting\n");
      exit(255);
    }
    reply_pointer->block = malloc(sizeof(struct data_block));
    if(!reply_pointer->block){
      printf("Allocation of reply packet failed: exiting\n");
      exit(255);
    }

    // data packet invariable fields (constants over chunks) 
    reply_pointer->opcode = OPCODE_DATA;
    reply_pointer->block->data = buffer;
    
    // source address pointer (for acks)
    socklen = sizeof(struct sockaddr_in));
    recv_pointer = malloc(socklen);
    
    //5: send all chunks
    aux = 512;
    for(uint16_t block_number = 1; aux == 512; ++block_number){
      // get chuck
      aux = fread(buffer, 1, 512, file_pointer);
      
      // prepare data packet (variable fields)
      reply_pointer->block_number = block_number;
      reply_pointer->block->dim = aux;

      // send packet
      ret = tftp_send(sock_fd, reply_pointer, client_address,
		      sizeof(struct sockaddr_in));
      
      // wait for ack
      message_pointer = tftp_message_alloc();
      ret = tftp_recv(sock_fd, message_pointer, recv_pointer,
		      &socklen);
      while(ret<0 ||
	    message_pointer->opcode != OPCODE_ACK ||
	    message_pointer->block_numer != block_number ||
	    recv_pointer->sin_addr!=client_pointer->sin_addr ||
	    recv_pointer->sin_port!=client_pointer->sin_port){
	
	// not the ack we were waiting for?
	socklen = sizeof(struct sockaddr_in);//might have been modified
	tftp_message_free(message_pointer); // it might have been a data packet
	message_pointer = tftp_message_alloc();
	ret = tftp_recv(sock_fd, message_pointer, recv_pointer,
			&socklen);
	
      }

      // we received the ack
      tftp_message_free(message_pointer);
    }

    //6: close socket
    close(sock_fd);
  }
}


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

  pthread_t workers[WORKERS_NUMBER];

  tftp_message * tftp_msg;
  void buffer[MAX_BUFFER_SIZE];

  ssize_t size;

  // random choice settings
  RAND_MAX = 65535 - 1024;
  srand(time(NULL));

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
    // allocate space for the request data
    tftp_msg = tftp_message_alloc();
    
    // wait for requests
    ret = tftp_recv(listener_sd, tftp_msg, (struct sockaddr *)client_address,
		     sizeof(struct sockaddr_in));
    
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

      // send error packet
      tftp_send(listener_sd, tftp_message, client_address,
		sizeof(struct sockaddr_in));
      
      // reset client_address, delete tftp_msg and continue
      memset(client_address, 0, sizeof(struct sockaddr_in));
      tftp_message_free((void*)tftp_message);
      continue ;
    }

    aux = 4;
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
