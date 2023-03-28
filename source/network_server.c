/*Grupo 17
* Catarina Lima 52787
* André Silva 52809
* João Oliveira 52839 */

#include "inet.h"
#include "message-private.h"
#include "network_server.h"
#include "signal.h"
#include <poll.h>

//fase 3
#define NFDESC 10    // Numero de sockets (uma para listening)
#define TIMEOUT 50  // em milisegundos
// --

int sockfd;

/* Função para preparar uma socket de receção de pedidos de ligação
 * num determinado porto.
 * Retornar descritor do socket (OK) ou -1 (erro).
 */
int network_server_init(short port) {
    struct sockaddr_in server;
    // Cria socket TCP
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erro ao criar socket");
        return -1;
    }

    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    // Preenche estrutura server com endereço(s) para associar (bind) à socket
    server.sin_family = AF_INET;
    server.sin_port = htons(port);               // Porta TCP
    server.sin_addr.s_addr = htonl(INADDR_ANY);  // Todos os endereços na máquina

    // Faz bind, ver observacao
    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Erro ao fazer bind");
        close(sockfd);
        return -1;
    }

    // Esta chamada diz ao SO que esta é uma socket para receber pedidos
    if (listen(sockfd, 0) < 0) {
        perror("Erro ao executar listen");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

/* Esta função deve:
 * - Aceitar uma conexão de um cliente;
 * - Receber uma mensagem usando a função network_receive;
 * - Entregar a mensagem de-serializada ao skeleton para ser processada;
 * - Esperar a resposta do skeleton;
 * - Enviar a resposta ao cliente usando a função network_send.
 */
int network_main_loop(int listening_socket) {
    struct sockaddr_in client;
    socklen_t size_client;

    // -- fase 3 --
    int nfds;
    int kfds, i;
    struct pollfd desc_set[NFDESC];  // Estrutura para file descriptors das sockets das ligacoes
    // ----

    for (i = 0; i < NFDESC; i++)
        desc_set[i].fd = -1;  // poll ignora estruturas com fd < 0

    // ----  adiciona listening_socket a desc_set
    desc_set[0].fd = listening_socket;
    desc_set[0].events = POLLIN;  // Vamos esperar ligacoes nesta socket
    // ------

    nfds = 1;

    while ((kfds = poll(desc_set, nfds, TIMEOUT)) >= 0) {  // kfds == 0 significa timeout sem eventos

        if (kfds > 0) {  // kfds e o numero de descritores com evento ou erro

            if ((desc_set[0].revents & POLLIN) && (nfds < NFDESC))                                                 // Pedido na listening socket ?
                if ((desc_set[nfds].fd = accept(desc_set[0].fd, (struct sockaddr *)&client, &size_client)) > 0) {  // Ligacao feita ?
                    desc_set[nfds].events = POLLIN;                                                                // Vamos esperar dados nesta socket
                    nfds++;
                }
            for (i = 1; i < nfds; i++) {  // Todas as ligacoes

                if (desc_set[i].revents & POLLIN) {  // Dados para ler

                    struct message_t *msg = network_receive(desc_set[i].fd);

                    if (msg == NULL) {
                        printf("cliente fechou a ligacao\n");
                        close(desc_set[i].fd);
                        desc_set[i].fd = -1;
                    } else {
                        printf("Recebeu pedido\n");

                        invoke(msg);

                        if (network_send(desc_set[i].fd, msg) == -1) {
                            close(desc_set[i].fd);
                            desc_set[i].fd = -1;
                            continue;           //continuar iteração do for
                        }
                        printf("Resposta enviada\n");
                    }
                    printf("à espera de um pedido\n");
                }

                if (desc_set[i].revents & POLLHUP || desc_set[i].revents & POLLERR) {
                    close(desc_set[i].fd);
                    desc_set[i].fd = -1;
                }
            }
        }
    }
    return 0;
}

/* Esta função deve:
 * - Ler os bytes da rede, a partir do client_socket indicado;
 * - De-serializar estes bytes e construir a mensagem com o pedido,
 *   reservando a memória necessária para a estrutura message_t.
 */
struct message_t *network_receive(int client_socket) {
    int len;
    uint8_t *lenS = (uint8_t *)&len;
    int rcvdBytes;

    if ((rcvdBytes = read_all(client_socket, lenS, sizeof(int))) < 0) {
        perror("Erro ao receber dados do cliente");
        return NULL;
        //close no loop principal
        //close(client_socket);
    }

    len = ntohl(len);

    if (len == -1)
        return NULL;

    uint8_t *buf = (uint8_t *)malloc(len);

    if ((rcvdBytes = read_all(client_socket, buf, len)) < 0) {
        perror("Erro ao receber dados do cliente");
        return NULL;
        //close no loop principal
        //close(client_socket);
    }

    struct message_t *msg = buf_to_message(&buf, len);

    free(buf);
    return msg;
}

/* Esta função deve:
 * - Serializar a mensagem de resposta contida em msg;
 * - Libertar a memória ocupada por esta mensagem;
 * - Enviar a mensagem serializada, através do client_socket.
 */
int network_send(int client_socket, struct message_t *msg) {
    uint8_t *buf;
    int len = message_to_buf(msg, &buf);

    if (len < 0) {
        perror("erro na serializacao para o cliente\n");
        return -1;
    }

    int lenToSend = htonl(len);
    uint8_t *lenS = (uint8_t *)&lenToSend;

    // Envia tamanho da msg
    int sentBytes;
    if ((sentBytes = write_all(client_socket, lenS, sizeof(int))) != sizeof(int)) {
        perror("Erro ao enviar resposta ao cliente");
        //close(client_socket);
        return -1;
    }
    // Envia msg
    if ((sentBytes = write_all(client_socket, buf, len)) != len) {
        perror("Erro ao enviar resposta ao cliente");
        //close(client_socket);
        return -1;
    }

    free(buf);
    return 0;
}

/* A função network_server_close() liberta os recursos alocados por
 * network_server_init().
 */
int network_server_close() {
    close(sockfd);
    return 0;
}
