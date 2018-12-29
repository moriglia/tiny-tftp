#ifndef TFTP_MESSAGES_H
#define TFTP_MESSAGES_H

#include <stdint.h>

struct data_block {
  void * data;
  uint16_t dim;
};

struct tftp_message {
  uint16_t opcode;
  union {
    uint16_t block_number;
    uint16_t error_code;
    uint16_t mode;
  };
  union {
    char * filename;
    char * error_message;
    struct data_block * block;
  };
};


#endif
