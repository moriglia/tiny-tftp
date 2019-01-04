#ifndef DEBUG
#define DEBUG 0
#endif

#include "headers/tftp_messages.h"
#include "headers/tftp_constants.h"
#include "headers/pack_message.h"
#include "headers/tftp_request_exchange_buffer.h"
#include "headers/tftp_message_alloc.h"
#include "headers/tftp_send.h"
#include "headers/tftp_recv.h"
#if DEBUG
#include "headers/tftp_display.h"
#endif
// network library headers
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

// other utility headers
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

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

char directory_path[200];

void * worker_thread(void* arg){
  int sock_fd, ret, aux;
  socklen_t socklen;
  struct tftp_message * message_pointer, * reply_pointer;
  struct sockaddr_in * client_pointer, worker_address, *recv_pointer;
#if DEBUG
  char addr_string[16];
#endif

  // reading variables
  FILE * file_pointer;
  char buffer[MAX_BUFFER_SIZE];


  // file path
  char file_string[MAX_BUFFER_SIZE];
  strncpy(file_string, directory_path, strlen(directory_path) + 1);
  
  // socket creation
  sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock_fd < 0){
    perror("Socket creation failed in thread. Exiting thread");
    ret = 255;
    pthread_exit(&ret);
  }

  // address configuration, binding once for each request to serve
  worker_address.sin_family = AF_INET;
  worker_address.sin_addr.s_addr = INADDR_ANY;

  for(;;){
    //1: withdraw request
    tftp_request_withdraw(&r, &message_pointer, &client_pointer);

    #if DEBUG
    printf("[Thrd] Withdrawed message:\n");
    tftp_display(message_pointer);
    #endif

    
    //2: choose a random port and bind to socket
    do {
      worker_address.sin_port = htons((time(NULL)*rand()*15485863)%64511 + 1024);
  
      ret = bind(sock_fd, (struct sockaddr*)&worker_address,
		 sizeof(struct sockaddr_in));
      
    } while(ret != 0);
    
    //3: open file to send
    strncpy(buffer, directory_path, strlen(directory_path) + 1);
    strncat(buffer, message_pointer->filename,
	    strlen(message_pointer->filename) + 1);
#if DEBUG
    printf("[Worker] sending %s\n", buffer);
#endif
    switch(message_pointer->mode){
    case MODE_NETASCII:
      file_pointer = fopen(buffer, "r");
      break;
    case MODE_OCTET:
      file_pointer = fopen(buffer, "rb");
      break;
    case MODE_MAIL:
    default:
      // not supported
      file_pointer = NULL;
    }

    //4: check for file existence
    if (!file_pointer){
      reply_pointer = tftp_message_alloc();
      
      if (reply_pointer){
	reply_pointer->opcode = OPCODE_ERROR;
	reply_pointer->error_code = TFTP_ERROR_FILE_NOT_FOUND;
	strncpy(buffer, "File not found: ", strlen("File not, found: ") + 1);
	strncat(buffer, message_pointer->filename,
		strlen(message_pointer->filename) + 1);
	reply_pointer->error_message = buffer;
#if DEBUG
	printf("File not found, sending error to client\n");
	tftp_display(reply_pointer);
#endif
	ret = tftp_send(sock_fd, reply_pointer, client_pointer,
			sizeof(struct sockaddr_in));
#if DEBUG
	printf("Error sent\n");
#endif
	// do not handle error
	reply_pointer->opcode = OPCODE_ACK;
	tftp_message_free(reply_pointer);
      }
      // not necessary any more
      tftp_message_free(message_pointer);
      
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
    socklen = sizeof(struct sockaddr_in);
    recv_pointer = malloc(socklen);
    
    //5: send all chunks
    aux = 512;
    for(uint16_t block_number = 1; aux == 512; ++block_number){
      // get chuck
      aux = fread(reply_pointer->block->data, 1, 512, file_pointer);
      
      // prepare data packet (variable fields)
      reply_pointer->block_number = block_number;
      reply_pointer->block->dim = aux;

      // send packet
#if DEBUG
      if (inet_ntop(AF_INET, (void*)&client_pointer->sin_addr,
	  addr_string, MAX_BUFFER_SIZE))
	printf("[Worker] sending data to client %s:%d\n",
	       addr_string, ntohs(client_pointer->sin_port));
      else
	printf("[Worker] sending data to client (conversion failed):%d\n",
	       ntohs(client_pointer->sin_port));
      tftp_display(reply_pointer);
#endif
      ret = tftp_send(sock_fd, reply_pointer, client_pointer,
		      sizeof(struct sockaddr_in));
      
      // wait for ack
      message_pointer = tftp_message_alloc();
      ret = tftp_recv(sock_fd, message_pointer, recv_pointer,
		      &socklen);
#if DEBUG
      if (inet_ntop(AF_INET, (void*)&recv_pointer->sin_addr,
	  buffer, MAX_BUFFER_SIZE))
	printf("[Worker] waiting for ack from client %s:%d\n",
	  buffer, client_pointer->sin_port);
      else
	printf("[Worker] waiting for ack from client (conversion failed):%d\n",
	  client_pointer->sin_port);
      tftp_display(message_pointer);
#endif
      while((ret<0) ||
	    (message_pointer->opcode != OPCODE_ACK) ||
	    (message_pointer->block_number != block_number) ||
	    (recv_pointer->sin_addr.s_addr != client_pointer->sin_addr.s_addr) ||
	    (recv_pointer->sin_port != client_pointer->sin_port)){
	
	// not the ack we were waiting for?
	socklen = sizeof(struct sockaddr_in);//might have been modified
	tftp_message_free(message_pointer); // might have been a data packet
	message_pointer = tftp_message_alloc();
	ret = tftp_recv(sock_fd, message_pointer, recv_pointer, &socklen);
#if DEBUG
	if (inet_ntop(AF_INET, (void*)&recv_pointer->sin_addr,
		      buffer, MAX_BUFFER_SIZE))
	  printf("[thrd] waiting for ack from client %s:%d\n",
		 buffer, client_pointer->sin_port);
	else
	  printf("[thrd] waiting for ack from client (conversion failed):%d\n",
		 client_pointer->sin_port);
	tftp_display(message_pointer);
#endif
      }

      // we received the ack
      tftp_message_free(message_pointer);
    }

    // free reply structure
    // note: we do not want to deallocate buffer: see tftp_message_free
    // to understand the reason for the following...
    reply_pointer->opcode = OPCODE_ERROR; 
    tftp_message_free(reply_pointer);
    
    //6: close socket
    close(sock_fd);
  }
}


int main(int argc, char** argv){
  // variables -----------------------------------------
  int ret,   // catch return status of functions
    //dir_len, // length of the directory string
    //buf_len,
    aux,
    socklen;
  
  int listener_sd;
  struct sockaddr_in listener_address, * client_address;

  int port;

#if DEBUG
  char presentation_address[16];
#endif
  
  pthread_t workers[WORKERS_NUMBER];

  struct tftp_message * tftp_msg;

  strncpy(directory_path, argv[2], strlen(argv[2]) + 1);

  // random choice settings
  srand(time(NULL));

  // ----------------------------------------------------
  if (argc != 3){
    printf("Usage: tftp_server <port> <directory>\n");
    printf("\t<port>\t\t1 - 65535\n");
    printf("\t<directory>\tDirectory with the files to serve\n");
    exit(255);
  }

  port = atoi(argv[1]);
  if (port > 65535 || port < 1){
    printf("Port out of range: %d\n", port);
    printf("Usage: tftp_server <port> <directory>\n");
    printf("\t<port>\t\t1 - 65535\n");
    printf("\t<directory>\tDirectory with the files to serve\n");
    exit(255);
  }
  //dir_len = strlen(argv[2]);
  

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
  ret = bind(listener_sd, (struct sockaddr*)&listener_address,
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
    do {
      errno = pthread_create(workers + i, NULL, worker_thread, NULL);
      perror("Thread creation");
    }while(errno);
  }



  // handle requests
  client_address = malloc(sizeof(struct sockaddr_in));
  memset(client_address, 0, sizeof(struct sockaddr_in));
  for(;;){
    // allocate space for the request data
    tftp_msg = tftp_message_alloc();
    
    // wait for requests
    socklen = sizeof(struct sockaddr_in);
    ret = tftp_recv(listener_sd, tftp_msg, client_address,
		    (socklen_t *)&socklen);
    #if DEBUG
    printf("Receivet TFTP packet:\n");
    tftp_display(tftp_msg);
    #endif

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
      #if DEBUG
      printf("Sending error packet for illegal operation\n");
      tftp_display(tftp_msg);
      #endif 
      tftp_send(listener_sd, tftp_msg, client_address,
		sizeof(struct sockaddr_in));
      printf("Illegal operation request. Discarded\n");
      
      // reset client_address, delete tftp_msg and continue
      memset(client_address, 0, sizeof(struct sockaddr_in));
      tftp_message_free((void*)tftp_msg);
      continue ;
    }

    #if DEBUG
    if (!inet_ntop(AF_INET, (void*)&client_address->sin_addr, presentation_address, sizeof(struct in_addr)))
      printf("Address conversion error, s_addr = %d, port= %d\n",
	     client_address->sin_addr.s_addr,
	     ntohs(client_address->sin_port));
    else
      printf("Request from %s:%d for file %s\n",
	     presentation_address, ntohs(client_address->sin_port),
	     tftp_msg->filename);
    #endif
    aux = 4;
    for (ret = tftp_request_deposit(&r, tftp_msg, client_address);
	 ret != TFTP_REB_DEPOSIT_SUCCESS && aux > 0; --aux){
      ret = tftp_request_deposit(&r, tftp_msg, client_address);
      sleep(1);
    }

    #if DEBUG
    printf("[main] Deposited:\n");
    tftp_display(tftp_msg);
    #endif

    if(ret != TFTP_REB_DEPOSIT_SUCCESS){
      printf("Dead workers or buffer full\nExiting\n");
      exit(255);
    }
    printf("Request handed to workers\n");

    client_address = malloc(sizeof(struct sockaddr_in));
    
  }
  
}
