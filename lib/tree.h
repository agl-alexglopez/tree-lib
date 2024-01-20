#ifndef TREE
#define TREE
#include <stdbool.h>
#include <stddef.h>

enum tree_link
{
  L = 0,
  R = 1
};

enum list_link
{
  P = 0,
  N = 1
};

struct dupnode
{
  struct dupnode *links[2];
  struct node *parent;
};

struct node
{
  struct node *links[2];
  struct dupnode *dups;
};

struct tree
{
  struct node *root;
  struct node nil;
  size_t size;
};

typedef enum
{
  LES = -1,
  EQL = 0,
  GRT = 1
} threeway_cmp;

/* To implement three way comparison in C you can try something
   like this:

     return (a > b) - (a < b);

   If such a comparison is not possible for your type you can simply
   return the value of the cmp enum directly with conditionals switch
   statements or whatever other comparison logic you choose. */
typedef threeway_cmp tree_cmp_fn (const struct node *key, const struct node *n,
                                  void *aux);

static inline struct dupnode *
as_dupnode (const struct node *d)
{
  return (struct dupnode *)(d);
}

static inline struct node *
as_node (const struct dupnode *d)
{
  return (struct node *)(d);
}

/* Mostly intended for debugging. Validates the underlying tree
   data structure with invariants that must hold regardless of
   interface. */
bool validate_tree (struct tree *t, tree_cmp_fn *cmp);
void print_tree (const struct node *root, const void *nil_and_tail);

#endif