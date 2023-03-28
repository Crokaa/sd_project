/*Grupo 17
* Catarina Lima 52787
* André Silva 52809
* João Oliveira 52839 */

#include "entry.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "data.h"
#include "string.h"

/* Função que cria uma entry, reservando a memória necessária e
 * inicializando-a com a string e o bloco de dados passados.
 */
struct entry_t *entry_create(char *key, struct data_t *data) {

    struct entry_t *entry = (struct entry_t *)malloc(sizeof(struct entry_t));


    if (entry == NULL) {
        return NULL;
    }

    if (key == NULL && data == NULL) {
        entry_initialize(entry);
        return entry;
    }

    entry->key = key;
    entry->value = data;

    return entry;
}

/* Função que inicializa os elementos de uma entry com o
 * valor NULL.
 */
void entry_initialize(struct entry_t *entry) {

    entry->key = NULL;
    entry->value = NULL;
}

/* Função que elimina uma entry, libertando a memória por ela ocupada
 */
void entry_destroy(struct entry_t *entry) {

    if (entry != NULL) {
        free(entry->key);
        data_destroy(entry->value);
        free(entry);
    }
}

/* Função que duplica uma entry, reservando a memória necessária para a
 * nova estrutura.
 */
struct entry_t *entry_dup(struct entry_t *entry) {

    if (entry == NULL)
        return NULL;

    if (entry->key == NULL && entry->value == NULL)
        //entra no initialize
        return entry_create(entry->key, entry->value);

    struct data_t *dupData = data_dup(entry->value);
    char *dupKey = strdup(entry->key);
    struct entry_t *entryDup = entry_create(dupKey, dupData);

    if (entryDup == NULL)
        return NULL;

    return entryDup;
}

/* Função que substitui o conteúdo de uma entrada entry_t.
*  Deve assegurar que destroi o conteúdo antigo da mesma.
*/
void entry_replace(struct entry_t *entry, char *new_key, struct data_t *new_value) {

    //se entry for null nao substituir nada
    if (entry == NULL) {
        return;
    }

    //initialize
    if (new_key == NULL && new_value == NULL) {
        entry_initialize(entry);
        return;
    }

    //se um dos valores for null, anular o replace
    if (new_key == NULL || new_value == NULL) {
        perror("Não possivel replace, entry dada fica igual");
        return;
    }

    free(entry->key);
    data_destroy(entry->value);

    entry->key = new_key;
    entry->value = new_value;
}

/* Função que compara duas entradas e retorna a ordem das mesmas.
*  Ordem das entradas é definida pela ordem das suas chaves.
*  A função devolve 0 se forem iguais, -1 se entry1<entry2, e 1 caso contrário.
*/
int entry_compare(struct entry_t *entry1, struct entry_t *entry2) {

    if (entry1 == NULL || entry2 == NULL) {
        errno = 1;
        perror("Não possivel comparar, alguma entry é null");
        exit(1);
    }

    //strcmp nao garante devolver 1 e -1, so >0 e <0
    int cmp = strcmp(entry1->key, entry2->key);

    if (cmp == 0)
        return 0;
    else if (cmp > 0)
        return 1;
    else
        return -1;
}
