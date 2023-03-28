#ifndef _TREE_SKEL_PRIVATE_H
#define _TREE_SKEL_PRIVATE_H

#include "zookeeper/zookeeper.h"

int tree_init_zoo_server(char *zoo_host, char *this_ip);

void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx);

#endif
