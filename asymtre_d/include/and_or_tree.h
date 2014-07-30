#ifndef TREE_H
#define TREE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dllist.h>

// tree node
typedef struct ao_node
{
  int type; // 0 = information node, 1 = schema node
  int andor; // 0 = and type, 1 = or type of the children nodes
  char *name; // the name of the node
  struct ao_node *parent; // current node's parent node
  Dllist children; // list of children nodes
  Dllist *schemalist; // associated with each information type
  int numways; // number of ways to generate this information type
  int processed; // 0: no, 1: yes
} *AO_node;
#endif
