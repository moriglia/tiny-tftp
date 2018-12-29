#ifndef PACK_MESSAGE_H
#define PACK_MESSAGE_H

int pack_message(const struct tftp_message * const src,
		 void* const dst,
		 int * const buffersize);

int unpack_message(const void * src,
		   struct tftp_message * const dst,
		   const int size);

/*
PARAMS
  src must be the pointer to a message structure
      as defined in tftp_message.h, casted to 
      a tftp_message pointer
  
  dst is the buffer to put the message bytes in

  buffersize must be the available size of the buffer

RETURN VALUES
  0: success
  1: invalid opcode
  2: insufficient buffer size
*/

#define PACK_SUCCESS 0
#define PACK_INVALID_OPCODE 1
#define PACK_INVALID_MODE_STRING 2
#define PACK_INSUFFICIENT_BUFFER_SIZE 3
#define PACK_NOT_IMPLEMENTED 4


#endif
