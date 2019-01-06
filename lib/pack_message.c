#ifndef PACK_MESSAGE_C
#define PACK_MESSAGE_C

#include "../headers/tftp_constants.h"
#include "../headers/tftp_messages.h"
#include "../headers/pack_message.h"

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdint.h>

int pack_message(const struct tftp_message * const src,
		 void * const dst,
		 int * const buffersize){
  
  int len_filename, len_mode, i;
  char * mode;
  uint8_t * c1, * c2;
  uint16_t * const opcode = (uint16_t*)dst;
  
  switch(src->opcode){

  case OPCODE_RRQ:
  case OPCODE_WRQ:
    
    len_filename = strlen(src->filename);
    switch(src->mode){
    case MODE_NETASCII:
      len_mode = 8;
      mode = malloc(9);
      strncpy(mode, MODE_NETASCII_STRING, 9);
      break;
    case MODE_OCTET:
      len_mode = 5;
      mode = malloc(6);
      strncpy(mode, MODE_OCTET_STRING, 6);
      break;
    case MODE_MAIL:
      len_mode = 4;
      mode = malloc(5);
      strncpy(mode, MODE_MAIL_STRING, 5);
	default:
	  return PACK_INVALID_MODE;
    }

    if((4 + len_filename + len_mode) > *buffersize){
      free(mode);
      // 4 = uint16_t + 2*'\0'
      return PACK_INSUFFICIENT_BUFFER_SIZE ;
    }    
    *buffersize = 4 + len_filename + len_mode;

    //field FILENAME
    strncpy((char*)dst + 2, src->filename, len_filename + 1);

    //field MODE
    strncpy((char*)dst + 3 + len_filename, mode, len_mode + 1);
    free(mode);

    break;
    
  case OPCODE_DATA:

    if(*buffersize < (4 + src->block->dim))
      // 4 = uint16_t * 2
      return PACK_INSUFFICIENT_BUFFER_SIZE ;

    //field DATA
    c1 = (uint8_t*)dst + 4;
    c2 = (uint8_t*)src->block->data;
    for(i = 0 ; i<src->block->dim; ++i)
      c1[i] = c2[i];

    // continue to OPCODE_ACK operations

  case OPCODE_ACK:
    
    if(*buffersize < 4) // ok even from OPCODE_DATA
      return PACK_INSUFFICIENT_BUFFER_SIZE ;

    *buffersize = 4;
    if (OPCODE_DATA == src->opcode)
      *buffersize += src->block->dim;

    // field BLOCK #
    opcode[1] = htons(src->block_number);

    break;

  case OPCODE_ERROR:

    len_filename  = strlen(src->error_message);

    if(*buffersize < ( 5 + len_filename))
      return PACK_INSUFFICIENT_BUFFER_SIZE;

    // field ERROR CODE
    opcode[1] = htons(src->error_code);

    strncpy((char*)dst + 4, src->error_message, len_filename + 1);

    break;

  default :
    return PACK_INVALID_OPCODE;
  }


  // field OPCODE for everyone
  // done here because the size must be checked first
  opcode[0]=htons(src->opcode);
  
  return PACK_SUCCESS;

}


int unpack_message(const void * src,
		   struct tftp_message * const dst,
		   const int size){
  
  uint16_t * opcode;
  uint8_t * c1, * c2;
  int len_filename, i;
  char * mode;
  
  opcode = (uint16_t*)src;

  dst->opcode = ntohs(*opcode);

  switch (dst->opcode){
  case OPCODE_RRQ:
  case OPCODE_WRQ:
    
    len_filename = strlen((char*)src + 2);
    
    //field filename
    dst->filename = malloc(len_filename + 1);
    strncpy(dst->filename, (char*)src + 2, len_filename + 1);

    //field mode
    mode = (char*)src + len_filename + 3;
    if(strcasecmp(mode, MODE_NETASCII_STRING) == 0)
      dst->mode = MODE_NETASCII;
    else if(strcasecmp(mode, MODE_OCTET_STRING) == 0)
      dst->mode = MODE_OCTET;
    else if(strcasecmp(mode, MODE_MAIL_STRING) == 0)
      dst->mode = MODE_MAIL;
    else
      return PACK_INVALID_MODE_STRING;

    return PACK_SUCCESS;

  case OPCODE_DATA:

    // field DATA
    dst->block = malloc(sizeof(struct data_block));
    dst->block->data = malloc(size - 4);
    c1 = (uint8_t*)dst->block->data;
    c2 = (uint8_t*)src + 4;
    for(i = 0; i < (size-4); ++i)
      c1[i] = c2[i];
    dst->block->dim = size - 4;

    // continue to ACK operations

  case OPCODE_ACK:
    
    // field BLOCK #
    dst->block_number = ntohs(*(opcode + 1));

    return PACK_SUCCESS;

  case OPCODE_ERROR:
    
    // field ERROR CODE
    dst->error_code = ntohs(opcode[1]);

    // field ERROR MESSAGE
    len_filename = strlen((char*)&opcode[2]);
    dst->error_message = malloc(len_filename + 1);
    strncpy(dst->error_message, (char*)&opcode[2], len_filename + 4);

    return PACK_SUCCESS;

  default:
    break;
  }

  return PACK_INVALID_OPCODE;
  
}

#endif
