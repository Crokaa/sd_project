/*Grupo 17
* Catarina Lima 52787
* André Silva 52809
* João Oliveira 52839 */

#include "data.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Função que cria um novo elemento de dados data_t e reserva a memória
 * necessária, especificada pelo parâmetro size
 */
struct data_t *data_create(int size) {

    if (size <= 0) {
        return NULL;
    }

    struct data_t *data = (struct data_t *)malloc(sizeof(struct data_t));
    if (data == NULL) {
        return NULL;
    }

    data->datasize = size;
    data->data = (void *) malloc(size);


    if (data->data == NULL) {
        return NULL;
    }

    return data;
}

/* Função idêntica à anterior, mas que inicializa os dados de acordo com
 * o parâmetro data.
 */
struct data_t *data_create2(int size, void *data) {

    if ( size < 0 || (size == 0 && data != NULL) || (size > 0 && data == NULL))
        return NULL;

    struct data_t *dataNova = (struct data_t *)malloc(sizeof(struct data_t));

    if (dataNova == NULL)
        return NULL;

    dataNova->datasize = size;
    dataNova->data = data;

    if (dataNova->data == NULL)
        return NULL;

    return dataNova;
}

/* Função que elimina um bloco de dados, apontado pelo parâmetro data,
 * libertando toda a memória por ele ocupada.
 */
void data_destroy(struct data_t *data) {

    if (data != NULL) {
        free(data->data);
        free(data);
    }
}

/* Função que duplica uma estrutura data_t, reservando a memória
 * necessária para a nova estrutura.
 */
struct data_t *data_dup(struct data_t *data) {

    if (data == NULL || data->datasize <= 0 || data->data == NULL){
        return NULL;
    }

    struct data_t *dataDup = data_create(data->datasize);

    memcpy(dataDup->data, data->data, data->datasize);

    if (dataDup == NULL)
        return NULL;

    return dataDup;
}

/* Função que substitui o conteúdo de um elemento de dados data_t.
*  Deve assegurar que destroi o conteúdo antigo do mesmo.
*/
void data_replace(struct data_t *data, int new_size, void *new_data) {

    if (data == NULL || new_size <= 0 || new_data == NULL)
        return;

    free(data->data);
    data->datasize = new_size;
    data->data = new_data;
}
