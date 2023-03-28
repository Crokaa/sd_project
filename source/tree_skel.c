/*Grupo 17
* Catarina Lima 52787
* André Silva 52809
* João Oliveira 52839 */

#include "message-private.h"
#include "tree_skel.h"
#include "tree_skel-private.h"
#include "client_stub.h"
#include "client_stub-private.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

struct tree_t *tree;

int last_assigned, op_count;
pthread_mutex_t queue_lock, tree_lock;
pthread_cond_t queue_not_empty;
struct task_t *queue_head;

pthread_t process;

#define ZDATALEN 1024 * 1024
static zhandle_t *zh;
char *this_server;
char *my_ip;
char *other_ip;
typedef struct String_vector zoo_string;
struct rtree_t *other_server;

/* Inicia o skeleton da árvore.
 * O main() do servidor deve chamar esta função antes de poder usar a
 * função invoke().
 * Retorna 0 (OK) ou -1 (erro, por exemplo OUT OF MEMORY)
 */
int tree_skel_init() {
    tree = tree_create();
    if (tree == NULL) {
        return -1;
    }
    last_assigned = 0;
    op_count = 0;

    if (pthread_mutex_init(&queue_lock, NULL) != 0) {
        perror("\nErro ao inicializar mutex\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_mutex_init(&tree_lock, NULL) != 0) {
        perror("\nErro ao inicializar mutex\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_init(&queue_not_empty, NULL) != 0) {
        perror("\nErro ao inicializar variável condicional\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&process, NULL, &process_task, NULL) != 0) {
        perror("\nErro ao criar thread.\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}

/* Liberta toda a memória e recursos alocados pela função tree_skel_init.
 */
void tree_skel_destroy() {
    if (pthread_detach(process) != 0) {
        perror("\nErro ao fazer detatch\n");
        // acho que pode continuar mesmo que haja erro
    }

    if (pthread_mutex_destroy(&queue_lock) != 0) {
        perror("\nErro ao destruir Mutex\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_mutex_destroy(&tree_lock) != 0) {
        perror("\nErro ao destruir Mutex\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_destroy(&queue_not_empty) != 0) {
        perror("\nErro ao destruir variável condicional\n");
        exit(EXIT_FAILURE);
    }

    tree_destroy(tree);
}

/* Executa uma operação na árvore (indicada pelo opcode contido em msg)
 * e utiliza a mesma estrutura message_t para devolver o resultado.
 * Retorna 0 (OK) ou -1 (erro, por exemplo, árvore nao incializada)
*/
int invoke(struct message_t *msg) {


    if (msg == NULL)
        return -1;

    if (tree == NULL) {
        msg->c_type = CT_BAD;
        msg->opcode = OP_ERROR;

        return -1;
    }

    int return_value = 0;


    switch (msg->opcode) {
        case OP_SIZE:
            msg->opcode = msg->opcode + 1;
            msg->c_type = CT_RESULT;

            pthread_mutex_lock(&tree_lock);
            msg->result = tree_size(tree);
            pthread_mutex_unlock(&tree_lock);
            break;

        case OP_GET:
            msg->opcode = msg->opcode + 1;
            msg->c_type = CT_VALUE;

            struct data_t *data;
            pthread_mutex_lock(&tree_lock);
            data = tree_get(tree, msg->key);
            pthread_mutex_unlock(&tree_lock);

            free(msg->data);
            if (data == NULL) {
                msg->data = NULL;
                msg->data_size = 0;
            } else {
                msg->data_size = data->datasize;
                msg->data = malloc(data->datasize);
                memcpy(msg->data, data->data, data->datasize);
            }

            data_destroy(data);
            break;

        case OP_DEL:
            msg->opcode = msg->opcode + 1;
            msg->c_type = CT_RESULT;

            msg->result = add_task_to_queue(msg);

            if (msg->result == -1) {
                msg->opcode = OP_ERROR;
                msg->c_type = CT_NONE;
                return_value = -1;
            }

            break;

        case OP_PUT:
            msg->opcode = msg->opcode + 1;
            msg->c_type = CT_RESULT;

            msg->result = add_task_to_queue(msg);

            if (msg->result == -1) {
                msg->opcode = OP_ERROR;
                msg->c_type = CT_NONE;
                return_value = -1;
            }

            break;

        case OP_GETKEYS:
            msg->opcode = msg->opcode + 1;
            msg->c_type = CT_KEYS;

            pthread_mutex_lock(&tree_lock);
            msg->keys = tree_get_keys(tree);
            msg->result = tree_size(tree);
            pthread_mutex_unlock(&tree_lock);
            break;

        case OP_HEIGHT:
            msg->opcode = msg->opcode + 1;
            msg->c_type = CT_RESULT;

            pthread_mutex_lock(&tree_lock);
            msg->result = tree_height(tree);
            pthread_mutex_unlock(&tree_lock);
            break;

        case OP_VERIFY:
            msg->opcode = msg->opcode + 1;
            msg->c_type = CT_RESULT;

            msg->result = verify(msg->result);

            if (msg->result == -1) {
                msg->opcode = OP_ERROR;
                msg->c_type = CT_NONE;
            }

            break;

        default:
            msg->opcode = OP_ERROR;
            msg->c_type = CT_BAD;
            return_value = -1;
    }

    return return_value;
}

/* Verifica se a operação identificada por op_n foi executada.
* Retorna 1 se operação foi executada, 0 se ainda não
* -1 se o servidor nem atribuiu essa operação a um cliente
*/
int verify(int op_n) {
    struct task_t *task = queue_head;

    while (task != NULL) {
        if (task->op_n == op_n)
            return 0;
        task = task->next;
    }

    return op_n < last_assigned ? 1 : -1;
}

/* Função do thread secundário que vai processar pedidos de escrita.
*/
void *process_task(void *params) {
    while (1) {
        pthread_mutex_lock(&queue_lock);

        while (queue_head == NULL)
            pthread_cond_wait(&queue_not_empty, &queue_lock);

        struct task_t *taskToDo = queue_head;
        queue_head = taskToDo->next;

        pthread_mutex_unlock(&queue_lock);

        int op_result;

        pthread_mutex_lock(&tree_lock);

        if (taskToDo->op == 0) {
            op_result = tree_del(tree, taskToDo->key);

            op_count++;

        } else if (taskToDo->op == 1) {
            op_result = tree_put(tree, taskToDo->key, taskToDo->data);

            if (op_result != -1)
                op_count++;
        }

        pthread_mutex_unlock(&tree_lock);

    }


    return NULL;
}

void destroy_task(struct task_t *task) {


    if (task != NULL) {
        free(task->key);
        data_destroy(task->data);
        task->next = NULL;
        free(task);

    }
}

/* Funçao que adiciona uma task ah lista de tarefas e retorna o numero da task
*/
int add_task_to_queue(struct message_t *msg) {
    struct task_t *task, *tptr;

    task = (struct task_t *)malloc(sizeof(struct task_t));

    if (task == NULL)
        return -1;

    if (msg->opcode == OP_PUT + 1)
        task->op = 1;
    else if (msg->opcode == OP_DEL + 1)
        task->op = 0;

    if (task->op == 0 && msg->key == NULL)
        return -1;

    if (task->op == 1 && (msg->key == NULL || msg->data == NULL))
        return -1;

    if (msg->key != NULL)
        task->key = strdup(msg->key);

    if (msg->data != NULL)
        task->data = data_create2(msg->data_size, strdup(msg->data));

    task->next = NULL;

    pthread_mutex_lock(&queue_lock);
    task->op_n = last_assigned;
    last_assigned += 1;

    if (queue_head == NULL) {
        queue_head = task;

    } else {
        tptr = queue_head;
        while (tptr->next != NULL)
            tptr = tptr->next;
        tptr->next = task;
    }

    pthread_cond_signal(&queue_not_empty); /* Avisa um bloqueado nessa condição */
    pthread_mutex_unlock(&queue_lock);

    if (strcmp(this_server, "primary") == 0) {
        if (msg->opcode == OP_PUT + 1){
            struct data_t *value = data_create2(msg->data_size, msg->data);
            struct entry_t *entry = entry_create(msg->key, value);
            return rtree_put(other_server, entry);
        }
        else if (msg->opcode == OP_DEL + 1){
            return rtree_del(other_server, msg->key);
        }
    }

    return task->op_n;
}

int tree_init_zoo_server(char *zoo_host, char *this_ip) {
    my_ip = this_ip;

    zh = zookeeper_init(zoo_host, NULL, 2000, 0, NULL, 0);

    if (zh == NULL) {
        fprintf(stderr, "Error connecting to ZooKeeper server!\n");
        return -1;
    }

    int checkKvstore = zoo_exists(zh, "/kvstore", 0, NULL);
    if (checkKvstore == ZNONODE) {
        printf("nao existe, criar kvstore e primary\n");

        zoo_create(zh, "/kvstore", NULL, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, NULL, 0);

        zoo_create(zh, "/kvstore/primary", this_ip, strlen(this_ip) + 1, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0);

        this_server = "primary";
    } else if (checkKvstore == ZOK) {
        int checkPrimary = zoo_exists(zh, "/kvstore/primary", 0, NULL);
        int checkBackup = zoo_exists(zh, "/kvstore/backup", 0, NULL);

        //esperar pelo backup se autopromover
        if(checkBackup == ZOK && checkPrimary == ZNONODE)
            sleep(3);

        //existem os 2, sair
        if (checkPrimary == ZOK && checkBackup == ZOK) {
            printf("Ja existem 2 servidores. A fechar\n");
            zookeeper_close(zh);
            return -1;

        //nao existe nenhum, criar primary
        } else if (checkPrimary == ZNONODE && checkBackup == ZNONODE) {

            zoo_create(zh, "/kvstore/primary", this_ip, strlen(this_ip) + 1, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0);
            printf("Servidor primary\n");
            this_server = "primary";

        //so existe primary, criar backup
        } else if (checkPrimary == ZOK) {

            zoo_create(zh, "/kvstore/backup", this_ip, strlen(this_ip) + 1, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0);
            printf("Servidor backup\n");
            this_server = "backup";
        }
    }

    zoo_string *children_list = (zoo_string *)malloc(sizeof(zoo_string));
    char *watcher_ctx = "ZooKeeper watch kvstore";
    if (ZOK != zoo_wget_children(zh, "/kvstore", child_watcher, watcher_ctx, children_list)) {
        printf("Error setting watch at %s!\n", "/kvstore");
    }


      if (other_ip != NULL)
          other_server = rtree_connect(other_ip);

    return 0;
}

void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx) {
    zoo_string *children_list = (zoo_string *)malloc(sizeof(zoo_string));
    if (state == ZOO_CONNECTED_STATE) {
        if (type == ZOO_CHILD_EVENT) {

            if (ZOK != zoo_wget_children(zh, "/kvstore", child_watcher, watcher_ctx, children_list)) {
                fprintf(stderr, "Error setting watch at %s!\n", "/kvstore");
            }

            if (strcmp("primary", this_server) == 0) {
                //sou primario, procurar backup
                for (int i = 0; i < children_list->count; i++) {
                    if (strcmp(children_list->data[i], "backup") == 0) {
                        int zoo_data_len = ZDATALEN;
                        char *backupData = malloc(ZDATALEN * sizeof(char));
                        zoo_get(zh, "/kvstore/backup", 0, backupData, &zoo_data_len, NULL);

                        //guardar ip dele, voltar a aceitar pedidos de escrita
                        other_ip = backupData;
                        other_server = rtree_connect(other_ip);
                        return;
                    }
                }

                //não aceita mais pedidos de escrita dos clientes até que volte a haver backup
                printf("Backup saiu\n");
                other_ip = NULL;
                return;

            } else if (strcmp("backup", this_server) == 0) {
                //sou backup
                int primaryLeft = 1;
                for (int i = 0; i < children_list->count; i++) {
                    if (strcmp(children_list->data[i], "primary") == 0)
                        primaryLeft = 0;
                }

                if (primaryLeft) {
                    //autopromover para primary
                    printf("Promoting to primary\n");
                    printf("Creating primary node\n");
                    int createPrimary = zoo_create(zh, "/kvstore/primary", my_ip,
                                                   strlen(my_ip) + 1, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0);
                    if (createPrimary == ZOK) {
                        this_server = "primary";
                        //delete backup node
                        zoo_delete(zh, "/kvstore/backup", -1);
                    }
                }
            }
        }
        free(children_list);
    }
}
