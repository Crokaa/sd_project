/*Grupo 17
* Catarina Lima 52787
* André Silva 52809
* João Oliveira 52839 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "client_stub-private.h"
#include "message-private.h"
#include "sdmessage.pb-c.h"
#include "inet.h"

/* Esta função deve:
 * - Obter o endereço do servidor (struct sockaddr_in) a base da
 *   informação guardada na estrutura rtree;
 * - Estabelecer a ligação com o servidor;
 * - Guardar toda a informação necessária (e.g., descritor do socket)
 *   na estrutura rtree;
 * - Retornar 0 (OK) ou -1 (erro).
 */
int network_connect(struct rtree_t *rtree) {
    struct sockaddr_in server = rtree->server;

    if ((rtree->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erro ao criar socket TCP");
        return -1;
    }

    int reuse = 1;
    if (setsockopt(rtree->socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    if (connect(rtree->socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Erro ao conectar-se ao servidor\n");
        close(rtree->socket);
        return -1;
    }

    return 0;
}

/* Esta função deve:
 * - Obter o descritor da ligação (socket) da estrutura rtree_t;
 * - Serializar a mensagem contida em msg;
 * - Enviar a mensagem serializada para o servidor;
 * - Esperar a resposta do servidor;
 * - De-serializar a mensagem de resposta;
 * - Retornar a mensagem de-serializada ou NULL em caso de erro.
 */
struct message_t *network_send_receive(struct rtree_t *rtree,
                                       struct message_t *msg) {
    if (rtree == NULL || msg == NULL)
        return NULL;

    uint8_t *buf;
    int len, nbytes, len2, rcvdBytes;
    len = message_to_buf(msg, &buf);
    int lenToSend = htonl(len);
    uint8_t *lenS = (uint8_t *)&lenToSend;

    //enviar primeiro o tamanho
    if ((nbytes = write_all(rtree->socket, lenS, sizeof(int))) != sizeof(int)) {
        perror("Erro ao enviar dados ao servidor1");
        return NULL;
    }

    if ((nbytes = write_all(rtree->socket, buf, len)) != len) {
        perror("Erro ao enviar dados ao servidor2");
        return NULL;
    }

    printf("À espera de resposta do servidor ...\n");

    uint8_t *buf_receive;

    uint8_t *lenS2 = (uint8_t *)&len2;
    if ((nbytes = read_all(rtree->socket, lenS2, sizeof(int))) != sizeof(int)) {
        perror("Erro ao receber dados do servidor");
        return NULL;
    };

    len2 = ntohl(len2);

    buf_receive = (uint8_t *)malloc(len2);
    if ((rcvdBytes = read_all(rtree->socket, buf_receive, len2)) != len2) {
        perror("Erro ao receber dados do cliente");
    }

    struct message_t *msg_rcv = buf_to_message(&buf_receive, len2);

    free(buf);
    free(buf_receive);

    return msg_rcv;
}

/* A função network_close() fecha a ligação estabelecida por
 * network_connect().
 */
int network_close(struct rtree_t *rtree) {
    return close(rtree->socket);
}
