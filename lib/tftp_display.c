#include "../headers/tftp_messages.h"
#include "../headers/tftp_constants.h"

#include <stdio.h>

void tftp_display(struct tftp_message * ptr){
  switch(ptr->opcode){
  case OPCODE_RRQ:
  case OPCODE_WRQ:
    printf("------ RRQ/WRQ ------\n");
    printf("Filename:\t%s\nMode:\t", ptr->filename);
    switch(ptr->mode){
    case MODE_NETASCII:
      printf(MODE_NETASCII_STRING);
      break;
    case MODE_OCTET:
      printf(MODE_OCTET_STRING);
      break;
    case MODE_MAIL:
      printf(MODE_MAIL_STRING);
      break;
    }
    printf("\n");
    break;

  case OPCODE_ERROR:
    printf("------ Error ------\n");
    printf("Error code: %d (msg: %s)\n", ptr->error_code, ptr->error_message);
    break;
    
  case OPCODE_DATA:
    printf("------ Data ------\n");
    printf("Block number:\t%d\n", ptr->block_number);
    printf("Block size:\t%d\n", ptr->block->dim);
    printf("Data: %s\n", (char*)ptr->block->data);
    break;
  case OPCODE_ACK:
    printf("------ ACK ------\n");
    printf("Block number:\t%d\n", ptr->block_number);
  }

  return;
}
