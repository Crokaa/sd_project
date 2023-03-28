/*Grupo 17
* Catarina Lima 52787
* André Silva 52809
* João Oliveira 52839 */

#include "tree.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "data.h"
#include "entry.h"
#include "tree-private.h"

/* Função para criar uma nova árvore tree vazia.
 * Em caso de erro retorna NULL.
 */
struct tree_t *tree_create() {
    struct tree_t *tree = (struct tree_t *)malloc(sizeof(struct tree_t));

    if (tree == NULL)
        return NULL;

    tree->data = NULL;
    tree->right = NULL;
    tree->left = NULL;
    return tree;
};

/* Função para libertar toda a memória ocupada por uma árvore.
 */
void tree_destroy(struct tree_t *tree) {
    if (tree == NULL)
        return;

    tree_destroy(tree->left);

    tree_destroy(tree->right);

    entry_destroy(tree->data);

    free(tree);
};

/* Função para adicionar um par chave-valor à árvore.
 * Os dados de entrada desta função deverão ser copiados, ou seja, a
 * função vai *COPIAR* a key (string) e os dados para um novo espaço de
 * memória que tem de ser reservado. Se a key já existir na árvore,
 * a função tem de substituir a entrada existente pela nova, fazendo
 * a necessária gestão da memória para armazenar os novos dados.
 * Retorna 0 (ok) ou -1 em caso de erro.
 */
int tree_put(struct tree_t *tree, char *key, struct data_t *value) {
    
    if (tree == NULL || key == NULL || value == NULL)
        return -1;

    //chegou a uma folha, put
    if (tree->data == NULL) {
        tree->data = entry_create(strdup(key), data_dup(value));
        return 0;
    }
    
    int cmp = strcmp(key, tree->data->key);

    if (cmp == 0) {
        //existe, substituir

        entry_replace(tree->data, strdup(key), data_dup(value));
        return 0;
    } else if (cmp > 0) {
        if (tree->right == NULL)
            tree->right = tree_create();

        return tree_put(tree->right, key, value);
    } else {
        if (tree->left == NULL)
            tree->left = tree_create();

        return tree_put(tree->left, key, value);
    }
}

/* Função para obter da árvore o valor associado à chave key.
 * A função deve devolver uma cópia dos dados que terão de ser
 * libertados no contexto da função que chamou tree_get, ou seja, a
 * função aloca memória para armazenar uma *CÓPIA* dos dados da árvore,
 * retorna o endereço desta memória com a cópia dos dados, assumindo-se
 * que esta memória será depois libertada pelo programa que chamou
 * a função.
 * Devolve NULL em caso de erro.
 */
struct data_t *tree_get(struct tree_t *tree, char *key) {
    if (tree == NULL || tree->data == NULL || key == NULL)
        return NULL;

    int cmp = strcmp(key, tree->data->key);

    if (cmp == 0)
        return data_dup(tree->data->value);
    else if (cmp > 0)
        return tree_get(tree->right, key);
    else
        return tree_get(tree->left, key);
}
/* Função que devolve a entrada maior mais pequena
*/
struct entry_t *findAndRemoveSmallestChild(struct tree_t *tree) {
    if (tree != NULL && tree->left != NULL)
        tree = tree->left;

    return tree->data;
}

/* Função para remover um elemento da árvore, indicado pela chave key.
 * Retorna a raiz da nova árvore corrigida
 */
struct tree_t *delete_recursion_aux(struct tree_t *tree, char *key) {
    if (tree == NULL || tree->data == NULL || key == NULL)
        return tree;

    int cmp = strcmp(key, tree->data->key);

    //valor a apagar aqui na raiz
    if (cmp == 0) {
        if (tree->left == NULL && tree->right == NULL) {
            tree_destroy(tree);
            return tree_create();
        }
        //ver numero de filhos
        else if (tree->left == NULL) {
            entry_destroy(tree->data);
            tree = tree->right;
            return tree;
        } else if (tree->right == NULL) {
            entry_destroy(tree->data);
            tree = tree->left;
            return tree;
        } else {
            if (tree->right->left == NULL) {
                //imediatamente a direita
                tree->data = tree->right->data;
                tree->right = tree->right->right;
                return tree;
            } else {
                //buscar o valor maior mais pequeno
                entry_destroy(tree->data);
                struct entry_t *entry = findAndRemoveSmallestChild(tree->right);
                tree->data = entry_dup(entry);
                tree->right = delete_recursion_aux(tree->right, entry->key);
                return tree;
            }
        }
    } else if (cmp > 0) {
        tree->right = delete_recursion_aux(tree->right, key);
        return tree;
    } else {  //(cmp < 0)
        tree->left = delete_recursion_aux(tree->left, key);
        return tree;
    }
}

/* Função para remover um elemento da árvore, indicado pela chave key,
 * libertando toda a memória alocada na respetiva operação tree_put.
 * Retorna 0 (ok) ou -1 (key not found).
 */
int tree_del(struct tree_t *tree, char *key) {
    struct data_t *data = tree_get(tree, key);
    if (data == NULL) {
        return -1;
    }

    data_destroy(data);
    *tree = *delete_recursion_aux(tree, key);

    return 0;
}

/* Função que devolve o número de elementos contidos na árvore.
 */
int tree_size(struct tree_t *tree) {
    if (tree == NULL || tree->data == NULL)
        return 0;

    return 1 + tree_size(tree->left) + tree_size(tree->right);
}

/* Função que devolve a altura da árvore.
 */
int tree_height(struct tree_t *tree) {
    if (tree == NULL || tree->data == NULL)
        return 0;

    return 1 + (tree_height(tree->left) > tree_height(tree->right) ? tree_height(tree->left) : tree_height(tree->right));
}

void get_keys_recursion_aux(struct tree_t *tree, char **keys, int *index) {
    if (tree == NULL || tree->data == NULL) {
        return;
    }

    keys[*index] = strdup(tree->data->key);

    *index += 1;
    get_keys_recursion_aux(tree->left, keys, index);
    get_keys_recursion_aux(tree->right, keys, index);
}

/* Função que devolve um array de char* com a cópia de todas as keys da
 * árvore, colocando o último elemento do array com o valor NULL e
 * reservando toda a memória necessária.
 */
char **tree_get_keys(struct tree_t *tree) {
    int index = 0;

    if (tree_size(tree) == 0) {
        return NULL;
    }

    //apontador para indice para saber ultima posicao
    int *ip;
    ip = &index;

    char **keys = malloc((tree_size(tree) + 1) * sizeof(char *));
    get_keys_recursion_aux(tree, keys, ip);

    keys[*ip] = NULL;
    return keys;
}

/* Função que liberta toda a memória alocada por tree_get_keys().
 */
void tree_free_keys(char **keys) {

    if(keys == NULL)
        return;

    int i = 0;
    while (keys[i] != NULL) {
        free(keys[i]);
        i++;
    }

    free(keys);
}
