/*Grupo 17
* Catarina Lima 52787
* André Silva 52809
* João Oliveira 52839 */

#include "data.h"
#include "entry.h"
#include "message-private.h"
#include "client_stub.h"
#include "client_stub-private.h"
#include "inet.h"
#include "network_client.h"

/* Remote tree. A definir pelo grupo em client_stub-private.h
 */
struct rtree_t;

/* Função para estabelecer uma associação entre o cliente e o servidor,
 * em que address_port é uma string no formato <hostname>:<port>.
 * Retorna NULL em caso de erro.
 */
struct rtree_t *rtree_connect(const char *address_port) {
    struct rtree_t *rtree = (struct rtree_t *)malloc(sizeof(struct rtree_t));

    if (rtree == NULL)
        return NULL;

    char *copy = strdup(address_port);

    char *host = strtok(copy, ":");

    copy = strtok(NULL, ":");

    char *port = strtok(copy, ":");

    if (strcmp(host, "localhost") == 0) {
        sprintf(host, "127.0.0.1");
    }

    // Preenche estrutura server com endereço do servidor para estabelecer
    // conexão
    rtree->server.sin_family = AF_INET;          // família de endereços
    rtree->server.sin_port = htons(atoi(port));  // Porta TCP

    if (inet_pton(AF_INET, host, &rtree->server.sin_addr) < 1) {  // Endereço IP
        printf("Erro ao converter IP\n");
        return NULL;
    }

    if (network_connect(rtree) < 0)
        return NULL;

    printf("Conectou\n");

    return rtree;
}

/* Termina a associação entre o cliente e o servidor, fechando a
 * ligação com o servidor e libertando toda a memória local.
 * Retorna 0 se tudo correr bem e -1 em caso de erro.
 */
int rtree_disconnect(struct rtree_t *rtree) {
    if (close(rtree->socket) == -1) {
        free(rtree);
        return -1;
    }

    free(rtree);

    return 0;
}

/* Função para adicionar um elemento na árvore.
 * Se a key já existe, vai substituir essa entrada pelos novos dados.
 * Devolve 0 (ok, em adição/substituição) ou -1 (problemas).
 */
int rtree_put(struct rtree_t *rtree, struct entry_t *entry) {
    if (rtree == NULL || entry == NULL) {
        return -1;
    }

    struct message_t *msg = (struct message_t *)malloc(sizeof(struct message_t));
    msg->opcode = OP_PUT;
    msg->c_type = CT_ENTRY;

    //entry
    msg->key = entry->key;
    msg->data_size = entry->value->datasize;
    msg->data = (char *)entry->value->data;

    struct message_t *receive = network_send_receive(rtree, msg);

    if (receive == NULL || receive->opcode == OP_ERROR) {
        return -1;
    }

    return receive->result;
}

/* Função para obter um elemento da árvore.
 * Em caso de erro, devolve NULL.
 */
struct data_t *rtree_get(struct rtree_t *rtree, char *key) {
    if (rtree == NULL || key == NULL) {
        return NULL;
    }

    struct message_t *msg = (struct message_t *)malloc(sizeof(struct message_t));
    msg->opcode = OP_GET;
    msg->c_type = CT_KEY;

    msg->key = key;

    struct message_t *receive = network_send_receive(rtree, msg);

    if (receive == NULL)
        return NULL;

    struct data_t *retorno = data_create2(receive->data_size, receive->data);

    if (receive->opcode == OP_ERROR)
        return NULL;

    return retorno;
}

/* Função para remover um elemento da árvore. Vai libertar
 * toda a memoria alocada na respetiva operação rtree_put().
 * Devolve: 0 (ok), -1 (key not found ou problemas).
 */
int rtree_del(struct rtree_t *rtree, char *key) {
    if (rtree == NULL || key == NULL) {
        return -1;
    }

    struct message_t *msg = (struct message_t *)malloc(sizeof(struct message_t));
    msg->opcode = OP_DEL;
    msg->c_type = CT_ENTRY;

    msg->key = key;

    struct message_t *receive = network_send_receive(rtree, msg);

    if (receive == NULL)
        return -1;

    if (receive->opcode == OP_ERROR)
        return -1;

    return receive->result;
}

/* Devolve o número de elementos contidos na árvore.
 */
int rtree_size(struct rtree_t *rtree) {
    if (rtree == NULL) {
        return -1;
    }

    struct message_t *msg = (struct message_t *)malloc(sizeof(struct message_t));
    msg->opcode = OP_SIZE;
    msg->c_type = CT_NONE;

    struct message_t *receive = network_send_receive(rtree, msg);

    if (receive == NULL)
        return -1;

    if (receive->opcode == OP_ERROR) {
        //
        return -1;
    }

    return receive->result;
}

/* Função que devolve a altura da árvore.
 */
int rtree_height(struct rtree_t *rtree) {
    if (rtree == NULL) {
        return -1;
    }

    struct message_t *msg = (struct message_t *)malloc(sizeof(struct message_t));
    msg->opcode = OP_HEIGHT;
    msg->c_type = CT_NONE;

    struct message_t *receive = network_send_receive(rtree, msg);

    if (receive == NULL)
        return -1;

    if (receive->opcode == OP_ERROR)
        return -1;

    return receive->result;
}

/* Devolve um array de char* com a cópia de todas as keys da árvore,
 * colocando um último elemento a NULL.
 */
char **rtree_get_keys(struct rtree_t *rtree) {
    if (rtree == NULL) {
        return NULL;
    }

    struct message_t *msg = (struct message_t *)malloc(sizeof(struct message_t));

    msg->opcode = OP_GETKEYS;
    msg->c_type = CT_NONE;

    struct message_t *receive = network_send_receive(rtree, msg);

    if (receive == NULL)
        return NULL;

    return receive->keys;
}

/* Liberta a memória alocada por rtree_get_keys().
 */
void rtree_free_keys(char **keys) {
    if (keys == NULL)
        return;

    int i = 0;
    while (keys[i] != NULL) {
        free(keys[i]);
    }
    free(keys);
}

/* Verifica se a operação identificada por op_n foi executada.
*/
int rtree_verify(struct rtree_t *rtree, int op_n) {
    if (rtree == NULL || op_n < 0) {
        return -1;
    }

    struct message_t *msg = (struct message_t *)malloc(sizeof(struct message_t));
    msg->opcode = OP_VERIFY;
    msg->c_type = CT_RESULT;

    msg->result = op_n;

    struct message_t *receive = network_send_receive(rtree, msg);

    if (receive == NULL)
        return -1;

    if (receive->opcode == OP_ERROR)
        return -1;

    return receive->result;
}
