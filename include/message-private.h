#ifndef _MESSAGE_PRIVATE_H
#define _MESSAGE_PRIVATE_H


#include <stdint.h>

typedef enum {
    OP_BAD = 0,
    OP_SIZE = 10 ,
    OP_DEL = 20,
    OP_GET = 30,
    OP_PUT = 40,
    OP_GETKEYS = 50,
    OP_HEIGHT = 60,
    OP_VERIFY = 70,
    OP_ERROR = 99
} Opcode;

typedef enum  {
    CT_BAD = 0,
    CT_KEY = 10,
    CT_VALUE = 20,
    CT_ENTRY = 30,
    CT_KEYS = 40,
    CT_RESULT = 50,
    CT_NONE = 60,
} C_type;

struct message_t {
    int opcode;
    int c_type;
    int result;
    int data_size;
    char* data;
    char* key;
    char ** keys;
};

int message_to_buf(struct message_t* msg, uint8_t **buf);

struct message_t *buf_to_message(uint8_t **buf, int len);

int write_all(int sock, uint8_t *buf, int len);

int read_all(int sock, uint8_t *buf, int len);

#endif
