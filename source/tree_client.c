/*Grupo 17
* Catarina Lima 52787
* André Silva 52809
* João Oliveira 52839 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "client_stub.h"
#include "entry.h"
#include "data.h"
#include <signal.h>
#include "zookeeper/zookeeper.h"

#define ZDATALEN 1024 * 1024
int zoo_data_len = ZDATALEN;

typedef struct String_vector zoo_string;

struct rtree_t *rtree_primary;
struct rtree_t *rtree_backup;

static zhandle_t *zh;

char *primary;
char *backup;

void catch_signal() {
    printf("Ocorreu um erro, a desligar\n");
    exit(0);
}

static void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx) {
    zoo_string *children_list = (zoo_string *)malloc(sizeof(zoo_string));
    int zoo_data_len = ZDATALEN;
    if (state == ZOO_CONNECTED_STATE) {
        if (type == ZOO_CHILD_EVENT) {
            /* Get the updated children and reset the watch */
            if (ZOK != zoo_wget_children(zh, "/kvstore", child_watcher, watcher_ctx, children_list)) {
                fprintf(stderr, "Error setting watch at %s!\n", "/kvstore");
            }

            // saiu servidor
            if (children_list->count < 2) {
                //1 servidor
                if (children_list->count == 1) {
                    // backup desconectou
                    if (strcmp(children_list->data[0], "primary") == 0){
                        rtree_disconnect(rtree_backup);
                        rtree_backup = NULL;
                    }

                    else{
                        rtree_disconnect(rtree_primary);
                        rtree_primary = NULL;
                    }

                } else {
                    //desconectar os dois
                    rtree_disconnect(rtree_backup);
                    rtree_disconnect(rtree_primary);
                }

                // conectaram
            } else {
                int b, p;
                b = (rtree_backup == NULL) ? 1 : 0;
                p = (rtree_primary == NULL) ? 1 : 0;

                for (int i = 0; i < children_list->count; i++) {
                    if (strcmp(children_list->data[i], "primary") == 0)
                        if (p) {
                            //int p1 =
                            zoo_get(zh, "/kvstore/primary", 0, primary, &zoo_data_len, NULL);
                            rtree_primary = rtree_connect(primary);
                        }

                    if (strcmp(children_list->data[i], "backup") == 0)
                        if (b) {
                            //int b1 =
                            zoo_get(zh, "/kvstore/backup", 0, backup, &zoo_data_len, NULL);
                            rtree_backup = rtree_connect(backup);
                        }
                }
            }
        }
        free(children_list);
    }
}

int main(int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGSEGV, catch_signal);

    if (argc != 2) {
        printf("Uso: ./tree-client <ip_servidor_Zoo>:<porto_Zoo>\n");
        printf("Exemplo de uso: ./tree-client 127.0.0.1:2181\n");
        return -1;
    }


    char *zoo_host = argv[1];  //ip:port zookeeper

    zh = zookeeper_init(zoo_host, NULL, 2000, 0, NULL, 0);

    if (zh == NULL) {
        fprintf(stderr, "Error connecting to ZooKeeper server!\n");
        exit(EXIT_FAILURE);
    }

    zoo_string *children_list = (zoo_string *)malloc(sizeof(zoo_string));
    char *watcher_ctx = "ZooKeeper watch kvstore";
    if (ZOK != zoo_wget_children(zh, "/kvstore", child_watcher, watcher_ctx, children_list)) {
        printf("Error setting watch at %s!\n", "/kvstore");
    }



    for (int i = 0; i < children_list->count; i++) {

        if (strcmp(children_list->data[i], "primary") == 0) {
            primary = malloc(ZDATALEN * sizeof(char));
            zoo_get(zh, "/kvstore/primary", 0, primary, &zoo_data_len, NULL);
        }

        if (strcmp(children_list->data[i], "backup") == 0) {
            backup = malloc(ZDATALEN * sizeof(char));
            zoo_get(zh, "/kvstore/backup", 0, backup, &zoo_data_len, NULL);
        }
    }

    if (primary == NULL || backup == NULL) {
        printf("Os servidores nao estao a receber pedidos, tente mais tarde\n");
        return -1;
    }

    rtree_primary = rtree_connect(primary);
    rtree_backup = rtree_connect(backup);

    if (rtree_primary == NULL || rtree_backup == NULL) {
        return -1;
    }

    char text[100];
    while (1) {
        printf("Enter a comand: \n");

        fgets(text, 100, stdin);
        text[strlen(text) - 1] = '\0';

        if (strcmp(text, "\0") == 0)
            continue;

        if (text == NULL) {
            return -1;
        }

        char *comando = strtok(text, " ");

        if (strcmp(comando, "quit") == 0) {
            break;
        }

        else if (rtree_backup == NULL || rtree_primary == NULL) {
            printf("Os servidores nao estao a receber pedidos, tente mais tarde\n");
        }

        else if (strcmp(comando, "put") == 0) {
            comando = strtok(NULL, " ");

            if (comando == NULL) {
                printf("Numero de argumentos errado\n");
                continue;
            }

            char *key = strdup(comando);

            comando = strtok(NULL, "\0");

            if (comando == NULL) {
                printf("Numero de argumentos errado\n");
                continue;
            }

            char *data = strdup(comando);

            struct data_t *value = data_create2(strlen(data) + 1, strdup(data));

            struct entry_t *entry = entry_create(key, value);

            printf("Codigo do pedido de put: %d\n", rtree_put(rtree_primary, entry));

        }

        else if (strcmp(comando, "get") == 0) {
            char *comando = strtok(NULL, "\0");

            if (comando == NULL) {
                printf("Numero de argumentos errado\n");
                continue;
            }

            struct data_t *data = rtree_get(rtree_backup, comando);

            if (data == NULL) {
                printf("%s não existe na tree\n", comando);
            } else {
                printf("Data: ");
                printf("%s\n", (char *)data->data);
            }

            data_destroy(data);

        }

        else if (strcmp(comando, "del") == 0) {
            char *comando = strtok(NULL, "\0");
            if (comando == NULL) {
                printf("Numero de argumentos errado\n");
                continue;
            }

            printf("Codigo do pedido de delete: %d\n", rtree_del(rtree_primary, comando));
        }

        else if (strcmp(comando, "size") == 0) {
            printf("Size: %d\n", rtree_size(rtree_backup));
        }

        else if (strcmp(comando, "height") == 0) {
            printf("Height: %d\n", rtree_height(rtree_backup));
        }

        else if (strcmp(comando, "getkeys") == 0) {
            char **keys = rtree_get_keys(rtree_backup);
            if (keys == NULL) {
                printf("Não existem keys\n");
            } else {
                int i = 0;
                while (keys[i] != NULL) {
                    printf("Chave %s\n", keys[i]);
                    i++;
                }
            }
        }

        else if (strcmp(comando, "verify") == 0) {
            char *comando = strtok(NULL, "\0");

            if (comando == NULL) {
                printf("Numero de argumentos errado\n");
                continue;
            }

            int r = rtree_verify(rtree_backup, atoi(comando));
            if (r == 1)
                printf("Operacao executada\n");
            else if (r == 0)
                printf("Operacao nao foi executada\n");
            else
                printf("Este codigo nao foi atribuido pelo servidor\n");
        }
    }

    rtree_disconnect(rtree_backup);
    rtree_disconnect(rtree_primary);
    return 0;
}
