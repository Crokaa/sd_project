/*Grupo 17
* Catarina Lima 52787
* André Silva 52809
* João Oliveira 52839 */

#include "message-private.h"
#include "inet.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sdmessage.pb-c.h>

int message_to_buf(struct message_t *msg, uint8_t **buf) {
    MessageT msg_proto;
    unsigned len;
    message_t__init(&msg_proto);

    switch (msg->opcode) {
        case 0:
            msg_proto.opcode = MESSAGE_T__OPCODE__OP_BAD;
            msg_proto.c_type = MESSAGE_T__C_TYPE__CT_BAD;
            msg_proto.offset = 0;
            break;

        case 10:

            msg_proto.opcode = MESSAGE_T__OPCODE__OP_SIZE;
            msg_proto.c_type = MESSAGE_T__C_TYPE__CT_NONE;
            msg_proto.offset = 0;
            break;
        case 11:

            msg_proto.opcode = MESSAGE_T__OPCODE__OP_SIZE;
            msg_proto.c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg_proto.offset = 1;
            break;
        case 20:
            msg_proto.opcode = MESSAGE_T__OPCODE__OP_DEL;
            msg_proto.c_type = MESSAGE_T__C_TYPE__CT_KEY;
            msg_proto.offset = 0;
            break;
        case 21:
            msg_proto.opcode = MESSAGE_T__OPCODE__OP_DEL;
            msg_proto.c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg_proto.offset = 1;
            break;
        case 30:
            msg_proto.opcode = MESSAGE_T__OPCODE__OP_GET;
            msg_proto.c_type = MESSAGE_T__C_TYPE__CT_KEY;
            msg_proto.offset = 0;
            break;
        case 31:
            msg_proto.opcode = MESSAGE_T__OPCODE__OP_GET;
            msg_proto.c_type = MESSAGE_T__C_TYPE__CT_VALUE;
            msg_proto.offset = 1;
            break;
        case 40:
            msg_proto.opcode = MESSAGE_T__OPCODE__OP_PUT;
            msg_proto.c_type = MESSAGE_T__C_TYPE__CT_ENTRY;
            msg_proto.offset = 0;
            break;
        case 41:
            msg_proto.opcode = MESSAGE_T__OPCODE__OP_PUT;
            msg_proto.c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg_proto.offset = 1;
            break;
        case 50:
            msg_proto.opcode = MESSAGE_T__OPCODE__OP_GETKEYS;
            msg_proto.c_type = MESSAGE_T__C_TYPE__CT_NONE;
            msg_proto.offset = 0;
            break;
        case 51:
            msg_proto.opcode = MESSAGE_T__OPCODE__OP_GETKEYS;
            msg_proto.c_type = MESSAGE_T__C_TYPE__CT_KEYS;
            msg_proto.offset = 1;
            break;
        case 60:
            msg_proto.opcode = MESSAGE_T__OPCODE__OP_HEIGHT;
            msg_proto.c_type = MESSAGE_T__C_TYPE__CT_NONE;
            msg_proto.offset = 0;
            break;
        case 61:
            msg_proto.opcode = MESSAGE_T__OPCODE__OP_HEIGHT;
            msg_proto.c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg_proto.offset = 1;
            break;
        case 70:
            msg_proto.opcode = MESSAGE_T__OPCODE__OP_VERIFY;
            msg_proto.c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg_proto.offset = 0;
            break;
        case 71:
            msg_proto.opcode = MESSAGE_T__OPCODE__OP_VERIFY;
            msg_proto.c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg_proto.offset = 1;
            break;

        default:
            msg_proto.opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg_proto.c_type = MESSAGE_T__C_TYPE__CT_NONE;
            msg_proto.offset = 0;
    }

    msg_proto.result = msg->result;
    msg_proto.data_size = msg->data_size;
    msg_proto.data = msg->data;
    msg_proto.key = msg->key;
    msg_proto.keys = msg->keys;

    if(msg_proto.opcode + msg_proto.offset == 51) {
      msg_proto.n_keys = msg->result;
    }

    len = message_t__get_packed_size(&msg_proto);

    *buf = (uint8_t *)malloc(len);

    if (*buf == NULL) {
        fprintf(stdout, "malloc error\n");
        return -1;
    }

    message_t__pack(&msg_proto, *buf);
    return len;
}

struct message_t *buf_to_message(uint8_t **buf, int len) {

    MessageT *msg_proto = message_t__unpack(NULL, len, *buf);

    struct message_t *msg = (struct message_t *)malloc(sizeof(struct message_t));

    msg->opcode = msg_proto->opcode + msg_proto->offset;
    msg->c_type = msg_proto->c_type;
    msg->result = msg_proto->result;
    msg->data_size = msg_proto->data_size;
    msg->data = strdup(msg_proto->data);
    msg->key = strdup(msg_proto->key);

    //copiar
    //msg->keys = msg_proto->keys;

    if(msg_proto->opcode + msg_proto->offset == 51) {

      if (msg->result == 0) {
        msg->keys = NULL;
      }
      else {

        msg->keys = malloc((msg->result + 1)* sizeof(char *));

        for(int j = 0; j < msg->result; j++) {
          msg->keys[j] = malloc(strlen(msg_proto->keys[j]));
          msg->keys[j] = strdup(msg_proto->keys[j]);
        }
      }

    }

    message_t__free_unpacked(msg_proto, NULL);

    return msg;
}

int write_all(int sock, uint8_t *buf, int len) {
    int bufsize = len;
    while (len > 0) {
        int res = write(sock, buf, len);
        if (res < 0) {
            if (errno == EINTR) continue;
            perror("write failed:");
            return res;
        }
        buf += res;
        len -= res;
    }

    return bufsize;
}

int read_all(int sock, uint8_t *buf, int len) {

    int bufsize = len;
    int res;
    
    while (len > 0) {

        if ((res  = read(sock, buf, len)) <= 0) {
            if (errno == EINTR) continue;

            // o fecho da socket do lado do cliente devolve 0
            // e o read nao reconhece o broken pipe como o write
            if(res == 0)
                errno = EPIPE;

            return -1;
        }
        buf += res;
        len -= res;
    }

    return bufsize;
}
