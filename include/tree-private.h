/*Grupo 17
* Catarina Lima 52787
* André Silva 52809
* João Oliveira 52839 */

#ifndef _TREE_PRIVATE_H
#define _TREE_PRIVATE_H

#include "tree.h"

struct tree_t {
	/** a preencher pelo grupo */

	struct entry_t *data;
	struct tree_t *left;
  	struct tree_t *right;
};

struct tree_t *delete_recursion_aux(struct tree_t *tree, char *key);

struct entry_t *getLargestChild(struct tree_t *tree);

void get_keys_recursion_aux(struct tree_t *tree, char **keys, int *index);

#endif
