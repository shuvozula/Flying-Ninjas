#include <and_or_tree.h>

// generate a new node
AO_node new_ao_node(int type, int andor, char *name, AO_node parent)
{
  AO_node node;

  node = (AO_node) malloc(sizeof(struct ao_node));
  node->type = type;
  node->andor = andor;
  node->name = strdup(name);
  node->parent = parent;
  node->children = new_dllist(); 
  node->schemalist = 0;
  node->numways = 0;
  node->processed = 0;
 
  return node;
}

// insert a child node to the and-or-tree
AO_node ao_insert_child(AO_node node, AO_node child)
{
  dll_append(node->children, new_jval_v((void *) child));
  return node;
}

// find a node with the key (name)
AO_node ao_find_child(AO_node head, char *name)
{
  AO_node node = 0, retv_node = 0;
  Dllist ptr;

  dll_traverse(ptr, head->children) {
    node = (AO_node) jval_v(dll_val(ptr));
    if (strcmp(node->name, name) == 0) 
      return node;
  } 
  dll_traverse(ptr, head->children) {
    node = (AO_node) jval_v(dll_val(ptr));
    retv_node = ao_find_child(node, name);
    if (retv_node != 0) break;
  }
  return retv_node;
}

// find my parent, return 0 if parent is root
AO_node ao_find_parent(AO_node node)
{
  if (node->parent == 0) {
    printf("root node\n");
    return 0;
  } else return node->parent;
}

// print the and-or-tree
void ao_printtree(AO_node head)
{
  Dllist ptr;
  AO_node node;

  //if (head->type == 0) 
    //printf("%s --> %d: ", head->name, head->numways);
  printf("%s: ", head->name);
  dll_traverse(ptr, head->children) {
    node = (AO_node) jval_v(dll_val(ptr));
    printf("%s, ", node->name);
  }
  printf("\n");

  dll_traverse(ptr, head->children) {
    node = (AO_node) jval_v(dll_val(ptr));
    ao_printtree(node);
  }
}
