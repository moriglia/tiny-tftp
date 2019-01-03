#include "headers/tftp_messages.h"
#include "headers/tftp_constants.h"
#include "headers/pack_message.h"
#include "headers/tftp_request_exchange_buffer.h"
#include "headers/tftp_message_alloc.h"
#include "headers/tftp_send.h"
#include "headers/tftp_recv.h"

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

#ifndef DEBUG
#define DEBUG 0
#endif

#define COMMAND_UNKNOWN -1
#define COMMAND_HELP 1
#define COMMAND_MODE 2
#define COMMAND_GET 3
#define COMMAND_QUIT 4

#define MODE_TXT 1
#define MODE_BIN 2

int mode;
char * remote_filename, * local_filename;
struct sockaddr_in server_address;//, client_address;

void print_help_message(){
  printf("Sono disponibili i seguenti comandi\n");
  printf("!help\t\tmostra l'elenco dei comandi disponibili\n");
  printf("!mode [txt|bin]\timposta la modalità di trasferimento dei file (testo o binaria)\n");
  printf("!get <server_file> <local_file>\trichiede al server il file <server_file> e lo salva localmente come <local_file>\n");
  printf("!quit\t\ttermina il client\n");
}

void print_usage(){
  printf("Usage: tftp_client <server-IP> <server-port>\n");
  printf("<server-IP>\tIP address of the server\n");
  printf("<server-port>\tlistening port of the server\n");
}

int prompt(){
  char command_string [128];
  printf("tftp> ");
  fgets(command_string, 128, stdin);

  if (strlen(command_string)<4){
    return COMMAND_UNKNOWN ;
  }
  
  if (strncmp(command_string, "!get", 4)==0){
    int ret;
    ret = sscanf(command_string, "!get %ms %ms", &remote_filename, &local_filename);
    switch(ret){
    case 2:
      return COMMAND_GET;
    case 1:
      free(remote_filename);
    default:
      return COMMAND_UNKNOWN;
    }
  }

  if (strlen(command_string)<5){
    return COMMAND_UNKNOWN ;
  }
  
  if (strncmp(command_string, "!help", 4)==0){
    return COMMAND_HELP ;
  }

  if (strncmp(command_string, "!mode", 4)==0){
    char * mode_string;
    int ret;
    ret = sscanf(command_string, "!mode %ms", &mode_string);
    if (ret==1){
      if(strncmp(mode_string, "txt", 3) == 0)
	mode = MODE_TXT;
      else if (strncmp(mode_string, "bin", 3) == 0)
	mode = MODE_BIN;
      else {
	free(mode_string);
	return COMMAND_UNKNOWN;
      }
      free(mode_string);
      return COMMAND_MODE;
    }
    return COMMAND_UNKNOWN;
  }
  
  if (strncmp(command_string, "!quit", 4)==0){
    printf("Grazie per averci scelto!\n");
    exit(0);
  }

  return COMMAND_UNKNOWN;
}

void handlerrq(struct tftp_message * tftp_msg){
  int sockfd, ret, aux;
  struct sockaddr_in client_address, incoming_address;
  FILE * fptr;
      
  // socket creation
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0){
    perror("Socket creation failed");
    exit(1);
  }
  
  // client address configuration
  client_address.sin_family = AF_INET;
  client_address.sin_addr.s_addr = INADDR_ANY;
  aux = 50;
  do {
    client_address.sin_port = htons( rand()*64511/RAND_MAX + 1024 );
    ret = bind(sockfd, (struct sockaddr *)&client_address,
	       sizeof(struct sockaddr_in));
  } while(ret != 0 && --aux);
  if (ret != 0){
    perror("Multiple binding attempts failed.");
    exit(1);
  }


  // open file
  if (mode == MODE_TXT)
    fptr = fopen(local_filename, "w");
  else
    fptr = fopen(local_filename, "wb");

  if (!fptr){
    printf("Error accessing the filesystem\nExiting\n");
    exit(1);
  }

  printf("Sending request for file %s\n", remote_filename);
  tftp_send(sockfd, tftp_msg, &server_address, sizeof(struct sockaddr_in));
  
  for(int due = 1; ; ++due){
    struct tftp_message * tftp_data;

    printf("Waiting for block number:\t%d\n", due);
    tftp_data = tftp_message_alloc();
    aux = sizeof(struct sockaddr_in);
    tftp_recv(sockfd, tftp_data, &incoming_address, (socklen_t *)&aux);
    while( incoming_address.sin_addr.s_addr != server_address.sin_addr.s_addr ||
	   incoming_address.sin_port != server_address.sin_port ||
	   tftp_data->opcode != OPCODE_DATA ||
	   tftp_data->block_number != due){
      tftp_message_free(tftp_data);
      tftp_data = tftp_message_alloc();
      aux = sizeof(struct sockaddr_in);
      tftp_recv(sockfd, tftp_data, &incoming_address, (socklen_t *)&aux);
    }

    printf("Writing block %d of %dB\n", due, tftp_data->block->dim);
    fwrite(tftp_data->block->data, 1, tftp_data->block->dim, fptr);

    printf("Acking block number:\t%d", tftp_data->block_number);
    tftp_data->opcode = OPCODE_ACK;
    tftp_send(sockfd, tftp_data, &server_address, sizeof(struct sockaddr_in));
    
    if (tftp_data->block->dim < 512)
      return;

    tftp_data->opcode = OPCODE_DATA;
    tftp_message_free(tftp_data);
  }
  
}

int main(int argc, char** argv){
  int ret;


  struct tftp_message tftp_msg;

  mode = MODE_TXT;
  
  if (argc < 3){
    print_usage();
    exit(1);
  }

  // server address configuration
  ret = atoi(argv[2]);
  if (ret <1 || ret >65535 ){
    printf("Port out of range [1-65535]: %d\n", ret);
    print_usage();
    exit(1);
  }
  server_address.sin_port = htons(ret);
  server_address.sin_family = AF_INET;
  ret = inet_pton(AF_INET, argv[1], (void*)&server_address.sin_addr);
  if (ret == 0) {
    printf("Formato IP non riconosciuto: %s\n", argv[1]);
    print_usage();
    exit(1);
  }

  print_help_message();

  for(;;){
    ret = prompt();

    switch(ret){
    case COMMAND_UNKNOWN:
    case COMMAND_HELP:
      print_help_message();
      continue;
    case COMMAND_MODE:
      if (mode == MODE_TXT)
	printf("Mode:\ttext\n");
      else
	printf("Mode:\tbin\n");
      continue; // nothing to do because mode is global and alredy set
    case COMMAND_GET:
    default:
      break;
    }

    tftp_msg.opcode = OPCODE_RRQ;
    tftp_msg.filename = malloc(strlen(remote_filename) + 1);
    strncpy(tftp_msg.filename, remote_filename, strlen(remote_filename) + 1);
    switch(mode){
    case MODE_TXT:
      tftp_msg.mode = MODE_NETASCII;
      break;
    case MODE_BIN:
      tftp_msg.mode = MODE_OCTET;
    }


    // handle request
    handlerrq(&tftp_msg);

    printf("Trasferimento completato\n");
    
  }

  return 0;
}
