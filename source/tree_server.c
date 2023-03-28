/*Grupo 17
* Catarina Lima 52787
* André Silva 52809
* João Oliveira 52839 */

#include "inet.h"
#include "network_server.h"
#include "signal.h"
#include "tree_skel-private.h"

void catch_signal() {
    printf("Ocorreu um erro interno no servidor, a desligar\n");
    network_server_close();
    exit(0);
}


int main(int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGSEGV, catch_signal);

    if (argc != 3) {
        printf("Uso: ./tree-server <porto_servidor> <ip_Zookeeper:port_Zookeeper>\n");
        printf("Exemplo de uso: ./tree-server 12345 localhost:2181\n");
        return -1;
    }

    char this_ip[30];
    char *zoo_host;

    sprintf(this_ip, "localhost:%s", argv[1]);
    int port = atoi(argv[1]);

    zoo_host = argv[2];

    int sockfd = network_server_init(port);

    if (sockfd == -1)
        return -1;

    if (tree_skel_init() == -1) {
        printf("Erro ao iniciar tree do servidor\n");
        return -1;
    }

    if (tree_init_zoo_server(zoo_host, this_ip) == -1) {
        printf("Erro ao iniciar servidor no zookeeper\n");
        return -1;
    }

    printf("Servidor à espera de dados\n");

    network_main_loop(sockfd);

    printf("closing\n");
    if (network_server_close() != -1)
        return -1;

    tree_skel_destroy();

    return 0;
}
