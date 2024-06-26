#include "pqueue.h"
#include "test.h"
#include "tree.h"

#include <stdio.h>

struct val
{
    int id;
    int val;
    struct pq_elem elem;
};

static enum test_result pq_test_insert_one(void);
static enum test_result pq_test_insert_three(void);
static enum test_result pq_test_insert_shuffle(void);
static enum test_result pq_test_struct_getter(void);
static enum test_result pq_test_insert_three_dups(void);
static enum test_result pq_test_read_max_min(void);
static enum test_result insert_shuffled(struct pqueue *, struct val[], size_t,
                                        int);
static size_t inorder_fill(int[], size_t, struct pqueue *);
static node_threeway_cmp val_cmp(const struct pq_elem *, const struct pq_elem *,
                                 void *);

#define NUM_TESTS (size_t)6
const test_fn all_tests[NUM_TESTS] = {
    pq_test_insert_one,        pq_test_insert_three,   pq_test_struct_getter,
    pq_test_insert_three_dups, pq_test_insert_shuffle, pq_test_read_max_min,
};

int
main()
{
    enum test_result res = PASS;
    for (size_t i = 0; i < NUM_TESTS; ++i)
    {
        const bool fail = all_tests[i]() == FAIL;
        if (fail)
        {
            res = FAIL;
        }
    }
    return res;
}

static enum test_result
pq_test_insert_one(void)
{
    struct pqueue pq;
    pq_init(&pq, val_cmp, NULL);
    struct val single;
    single.val = 0;
    pq_push(&pq, &single.elem);
    CHECK(pq_empty(&pq), false, bool, "%b");
    CHECK(pq_entry(pq_root(&pq), struct val, elem)->val == single.val, true,
          bool, "%b");
    return PASS;
}

static enum test_result
pq_test_insert_three(void)
{
    struct pqueue pq;
    pq_init(&pq, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = i;
        pq_push(&pq, &three_vals[i].elem);
        CHECK(validate_tree(&pq.t), true, bool, "%b");
        CHECK(pq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(pq_size(&pq), 3, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_struct_getter(void)
{
    struct pqueue pq;
    pq_init(&pq, val_cmp, NULL);
    struct pqueue pq_tester_clone;
    pq_init(&pq_tester_clone, val_cmp, NULL);
    struct val vals[10];
    struct val tester_clone[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        tester_clone[i].val = i;
        pq_push(&pq, &vals[i].elem);
        pq_push(&pq_tester_clone, &tester_clone[i].elem);
        CHECK(validate_tree(&pq.t), true, bool, "%b");
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our get
           to uncorrupted data. */
        const struct val *get
            = pq_entry(&tester_clone[i].elem, struct val, elem);
        CHECK(get->val, vals[i].val, int, "%d");
    }
    CHECK(pq_size(&pq), 10ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_insert_three_dups(void)
{
    struct pqueue pq;
    pq_init(&pq, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = 0;
        pq_push(&pq, &three_vals[i].elem);
        CHECK(validate_tree(&pq.t), true, bool, "%b");
        CHECK(pq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(pq_size(&pq), 3ULL, size_t, "%zu");
    return PASS;
}

static node_threeway_cmp
val_cmp(const struct pq_elem *a, const struct pq_elem *b, void *aux)
{
    (void)aux;
    struct val *lhs = pq_entry(a, struct val, elem);
    struct val *rhs = pq_entry(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

static enum test_result
pq_test_insert_shuffle(void)
{
    struct pqueue pq;
    pq_init(&pq, val_cmp, NULL);
    /* Math magic ahead... */
    const size_t size = 50;
    const int prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&pq, vals, size, prime), PASS, enum test_result,
          "%d");
    const struct val *max = pq_entry(pq_const_max(&pq), struct val, elem);
    CHECK(max->val, size - 1, int, "%d");
    const struct val *min = pq_entry(pq_const_min(&pq), struct val, elem);
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &pq), size, size_t, "%zu");
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i], int, "%d");
    }
    return PASS;
}

static enum test_result
pq_test_read_max_min(void)
{
    struct pqueue pq;
    pq_init(&pq, val_cmp, NULL);
    struct val vals[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        pq_push(&pq, &vals[i].elem);
        CHECK(validate_tree(&pq.t), true, bool, "%b");
        CHECK(pq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(pq_size(&pq), 10ULL, size_t, "%zu");
    const struct val *max = pq_entry(pq_const_max(&pq), struct val, elem);
    CHECK(max->val, 9, int, "%d");
    const struct val *min = pq_entry(pq_const_min(&pq), struct val, elem);
    CHECK(min->val, 0, int, "%d");
    return PASS;
}

static enum test_result
insert_shuffled(struct pqueue *pq, struct val vals[], const size_t size,
                const int larger_prime)
{
    /* Math magic ahead so that we iterate over every index
       eventually but in a shuffled order. Not necessarily
       randome but a repeatable sequence that makes it
       easier to debug if something goes wrong. Think
       of the prime number as a random seed, kind of. */
    size_t shuffled_index = larger_prime % size;
    for (size_t i = 0; i < size; ++i)
    {
        vals[shuffled_index].val = (int)shuffled_index;
        pq_push(pq, &vals[shuffled_index].elem);
        CHECK(pq_size(pq), i + 1, size_t, "%zu");
        CHECK(validate_tree(&pq->t), true, bool, "%b");
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(pq_size(pq), size, size_t, "%zu");
    return PASS;
}

/* Iterative inorder traversal to check the heap is sorted. */
static size_t
inorder_fill(int vals[], size_t size, struct pqueue *pq)
{
    if (pq_size(pq) != size)
    {
        return 0;
    }
    size_t i = 0;
    for (struct pq_elem *e = pq_rbegin(pq); e != pq_end(pq);
         e = pq_rnext(pq, e))
    {
        vals[i++] = pq_entry(e, struct val, elem)->val;
    }
    return i;
}
