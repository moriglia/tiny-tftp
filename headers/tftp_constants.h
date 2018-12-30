#ifndef TFTP_CONSTANTS_H
#define TFTP_CONSTANTS_H

#include <arpa/inet.h>
#include <stdint.h>

// opcodes -----------------------------------------------------------
// UNDER THIS VALUE ALL OPCODES ARE NOT VALID
#define OPCODE_INVALID_LOW 0
// ABOVE THIS VALUE ALL OPCODES ARE NOT VALID
#define OPCODE_INVALID_HIGH 6

// valid opcodes
#define OPCODE_RRQ 1
#define OPCODE_WRQ 2
#define OPCODE_DATA 3
#define OPCODE_ACK 4
#define OPCODE_ERROR 5


// transfer modes ----------------------------------------------------
// numeric codes are not transmitted
#define MODE_NETASCII 1
#define MODE_OCTET 2
#define MODE_MAIL 3

// string values to copy in buffers and to compare received values to.
#define MODE_NETASCII_STRING "NETASCII"
#define MODE_OCTET_STRING "OCTET"
#define MODE_MAIL_STRING "MAIL"

// error codes ---------------------------------------------------------
#define TFTP_ERROR_GENERIC 0
#define TFTP_ERROR_FILE_NOT_FOUND 1
#define TFTP_ERROR_ACCESS_VIOLATION 2
#define TFTP_ERROR_ALLOCATION_EXCEEDED 3
#define TFTP_ERROR_ILLEGAL 4
#define TFTP_ERROR_URECOGNIZED_TRANSFER 5
#define TFTP_ERROR_FILE_EXISTS 6
#define TFTP_ERROR_USER_NOT_FOUND 7

#endif
