#ifndef PACK_MESSAGE_H
#define PACK_MESSAGE_H

int pack_message(const struct tftp_message * const src,
		 void* const dst,
		 int * const buffersize);
		 /*
		 PARAMS
		   src must be the pointer to a message structure
		       as defined in tftp_message.h

		   dst is the buffer to put the message bytes in

		   buffersize must be the available size of the buffer
		 */

int unpack_message(const void * src,
		   struct tftp_message * const dst,
		   const int size);
		   /*
		   PARAMS
		     dst must be the pointer to a message structure
		         as defined in tftp_message.h, which will be
				 filled with the buffer src content decoded

		     src is the buffer to get the message bytes from

		     size must be equal to the buffer size.
		   */

/*
RETURN VALUES
  See the PACK_<description> variables below
*/

#define PACK_SUCCESS 0
#define PACK_INVALID_OPCODE 1
#define PACK_INVALID_MODE 2
#define PACK_INVALID_MODE_STRING 3
#define PACK_INSUFFICIENT_BUFFER_SIZE 4
#define PACK_NOT_IMPLEMENTED 5


#endif
