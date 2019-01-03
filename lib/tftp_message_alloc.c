#include <stdlib.h>
#include "../headers/tftp_messages.h"
#include "../headers/tftp_constants.h"

void * tftp_message_alloc(){
  struct tftp_message * ptr = malloc(sizeof(struct tftp_message));
  ptr->opcode = OPCODE_INVALID_LOW;
  // to prevent accidental "OPCODE_DATA" (would be dangerous in ..._free function)
  return ptr;
}

void tftp_message_free(void * ptr){
  struct tftp_message * msg = (struct tftp_message *)ptr;

  switch(msg->opcode){

  case OPCODE_DATA:
    free(msg->block->data);
    
  case OPCODE_ERROR:
  case OPCODE_RRQ:
  case OPCODE_WRQ:
    free(msg->block); // or error_message or filename

  case OPCODE_ACK:
  default:
    free(ptr);
  }
}
