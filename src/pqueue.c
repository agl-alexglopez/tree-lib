#include "pqueue.h"

#include <stdbool.h>
#include <stddef.h>

/*=========================  Function Prototypes   ==========================*/

static struct pq_elem *fair_merge(struct pqueue *, struct pq_elem *old,
                                  struct pq_elem *new);
static void link_child(struct pq_elem *parent, struct pq_elem *child);
static void init_node(struct pq_elem *);
static size_t traversal_size(struct pq_elem const *);
static bool has_valid_links(struct pqueue const *, struct pq_elem const *parent,
                            struct pq_elem const *child);
static struct pq_elem *delete(struct pqueue *, struct pq_elem *);
static struct pq_elem *delete_min(struct pqueue *, struct pq_elem *);
static void clear_node(struct pq_elem *);
static void cut_child(struct pq_elem *);

/*=========================  Interface Functions   ==========================*/

struct pq_elem const *
pq_front(struct pqueue const *const ppq)
{
    return ppq->root;
}

void
pq_push(struct pqueue *const ppq, struct pq_elem *const e)
{
    if (!e || !ppq)
    {
        return;
    }
    init_node(e);
    ppq->root = fair_merge(ppq, ppq->root, e);
    ++ppq->sz;
}

struct pq_elem *
pq_pop(struct pqueue *const ppq)
{
    if (!ppq->root)
    {
        return NULL;
    }
    struct pq_elem *const popped = ppq->root;
    ppq->root = delete_min(ppq, ppq->root);
    ppq->sz--;
    clear_node(popped);
    return popped;
}

struct pq_elem *
pq_erase(struct pqueue *const ppq, struct pq_elem *const e)
{
    if (!ppq->root || !e->next_sibling || !e->prev_sibling)
    {
        return NULL;
    }
    ppq->root = delete (ppq, e);
    ppq->sz--;
    clear_node(e);
    return e;
}

void
pq_clear(struct pqueue *const ppq, pq_destructor_fn *fn)
{
    while (!pq_empty(ppq))
    {
        fn(pq_pop(ppq));
    }
}

bool
pq_empty(struct pqueue const *const ppq)
{
    return !ppq->sz;
}

size_t
pq_size(struct pqueue const *const ppq)
{
    return ppq->sz;
}

/* This is a difficult function. Without knowing if this new value is greater
   or less than the previous we must always perform a delete and reinsert if
   the value has not broken total order with the parent. It is not sufficient
   to check if the value has exceeded the value of the first left child as
   any sibling of that left child may be bigger than or smaller than that
   left child value. */
bool
pq_update(struct pqueue *const ppq, struct pq_elem *const e,
          pq_update_fn *const fn, void *const aux)
{
    if (!e->next_sibling || !e->prev_sibling)
    {
        return false;
    }
    fn(e, aux);
    if (e->parent && ppq->cmp(e, e->parent, ppq->aux) == ppq->order)
    {
        cut_child(e);
        ppq->root = fair_merge(ppq, ppq->root, e);
        return true;
    }
    ppq->root = delete (ppq, e);
    init_node(e);
    ppq->root = fair_merge(ppq, ppq->root, e);
    return true;
}

/* Preferable to use this function if it is known the value is increasing.
   Much more efficient. */
bool
pq_increase(struct pqueue *const ppq, struct pq_elem *const e, pq_update_fn *fn,
            void *aux)
{
    if (!e->next_sibling || !e->prev_sibling)
    {
        return false;
    }
    if (ppq->order == PQGRT)
    {
        fn(e, aux);
        cut_child(e);
    }
    else
    {
        ppq->root = delete (ppq, e);
        fn(e, aux);
        init_node(e);
    }
    ppq->root = fair_merge(ppq, ppq->root, e);
    return true;
}

/* Preferable to use this function if it is known the value is decreasing.
   Much more efficient. */
bool
pq_decrease(struct pqueue *const ppq, struct pq_elem *const e, pq_update_fn *fn,
            void *aux)
{
    if (!e->next_sibling || !e->prev_sibling)
    {
        return false;
    }
    if (ppq->order == PQLES)
    {
        fn(e, aux);
        cut_child(e);
    }
    else
    {
        ppq->root = delete (ppq, e);
        fn(e, aux);
        init_node(e);
    }
    ppq->root = fair_merge(ppq, ppq->root, e);
    return true;
}

bool
pq_validate(struct pqueue const *const ppq)
{
    if (ppq->root && ppq->root->parent)
    {
        return false;
    }
    if (!has_valid_links(ppq, NULL, ppq->root))
    {
        return false;
    }
    if (traversal_size(ppq->root) != ppq->sz)
    {
        return false;
    }
    return true;
}

enum pq_threeway_cmp
pq_order(struct pqueue const *const ppq)
{
    return ppq->order;
}

/*========================   Static Helpers   ================================*/

static void
init_node(struct pq_elem *e)
{
    e->left_child = e->parent = NULL;
    e->next_sibling = e->prev_sibling = e;
}

static void
clear_node(struct pq_elem *e)
{
    e->left_child = e->next_sibling = e->prev_sibling = e->parent = NULL;
}

static void
cut_child(struct pq_elem *child)
{
    child->next_sibling->prev_sibling = child->prev_sibling;
    child->prev_sibling->next_sibling = child->next_sibling;
    if (child->parent && child == child->parent->left_child)
    {
        if (child->next_sibling == child)
        {
            child->parent->left_child = NULL;
        }
        else
        {
            child->parent->left_child = child->next_sibling;
        }
    }
    child->parent = NULL;
}

static struct pq_elem *delete(struct pqueue *ppq, struct pq_elem *root)
{
    if (ppq->root == root)
    {
        return delete_min(ppq, root);
    }
    cut_child(root);
    return fair_merge(ppq, ppq->root, delete_min(ppq, root));
}

static struct pq_elem *
delete_min(struct pqueue *ppq, struct pq_elem *root)
{
    if (!root->left_child)
    {
        return NULL;
    }
    struct pq_elem *const eldest = root->left_child->next_sibling;
    struct pq_elem *accumulator = root->left_child->next_sibling;
    struct pq_elem *cur = root->left_child->next_sibling->next_sibling;
    while (cur != eldest && cur->next_sibling != eldest)
    {
        struct pq_elem *next = cur->next_sibling;
        struct pq_elem *next_cur = cur->next_sibling->next_sibling;
        next->next_sibling = next->prev_sibling = NULL;
        cur->next_sibling = cur->prev_sibling = NULL;
        accumulator = fair_merge(ppq, accumulator, fair_merge(ppq, cur, next));
        cur = next_cur;
    }
    /* This covers the odd or even case for number of pairings. */
    root = cur != eldest ? fair_merge(ppq, accumulator, cur) : accumulator;
    /* The root is always alone in its circular list at the end of merges. */
    root->next_sibling = root->prev_sibling = root;
    root->parent = NULL;
    return root;
}

static inline struct pq_elem *
fair_merge(struct pqueue *const ppq, struct pq_elem *const old,
           struct pq_elem *const new)
{
    if (!old || !new || old == new)
    {
        return old ? old : new;
    }
    if (ppq->cmp(new, old, ppq->aux) == ppq->order)
    {
        link_child(new, old);
        return new;
    }
    link_child(old, new);
    return old;
}

/* Oldest nodes shuffle down, new drops in to replace.
         a       a       a
        ╱   ->  ╱   ->  ╱
      ┌b┐     ┌c─b┐   ┌d─c─b┐
      └─┘     └───┘   └─────┘ */
static inline void
link_child(struct pq_elem *const parent, struct pq_elem *const child)
{
    if (parent->left_child)
    {
        child->next_sibling = parent->left_child->next_sibling;
        child->prev_sibling = parent->left_child;
        parent->left_child->next_sibling->prev_sibling = child;
        parent->left_child->next_sibling = child;
    }
    else
    {
        child->next_sibling = child->prev_sibling = child;
    }
    parent->left_child = child;
    child->parent = parent;
}

/*========================     Validation    ================================*/

/* NOLINTBEGIN(*misc-no-recursion) */

static size_t
traversal_size(struct pq_elem const *const root)
{
    if (!root)
    {
        return 0;
    }
    size_t sz = 0;
    bool sibling_ring_lapped = false;
    struct pq_elem const *cur = root;
    while (!sibling_ring_lapped)
    {
        sz += 1 + traversal_size(cur->left_child);
        cur = cur->next_sibling;
        sibling_ring_lapped = cur == root;
    }
    return sz;
}

static bool
has_valid_links(struct pqueue const *const ppq,
                struct pq_elem const *const parent,
                struct pq_elem const *const child)
{
    if (!child)
    {
        return true;
    }
    bool sibling_ring_lapped = false;
    struct pq_elem const *cur = child;
    enum pq_threeway_cmp const wrong_order
        = ppq->order == PQLES ? PQGRT : PQLES;
    while (!sibling_ring_lapped)
    {
        if (!cur)
        {
            return false;
        }
        if (parent && child->parent != parent)
        {
            return false;
        }
        if (parent && parent->left_child != child->parent->left_child)
        {
            return false;
        }
        if (child->next_sibling->prev_sibling != child
            || child->prev_sibling->next_sibling != child)
        {
            return false;
        }
        if (parent && (ppq->cmp(parent, cur, ppq->aux) == wrong_order))
        {
            return false;
        }
        if (!has_valid_links(ppq, cur, cur->left_child))
        {
            return false;
        }
        cur = cur->next_sibling;
        sibling_ring_lapped = cur == child;
    }
    return true;
}

/* NOLINTEND(*misc-no-recursion) */
