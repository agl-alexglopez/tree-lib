#include "pqueue.h"
#include "test.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct val
{
    int id;
    int val;
    struct pq_elem elem;
};

static enum test_result pq_test_insert_remove_four_dups(void);
static enum test_result pq_test_insert_erase_shuffled(void);
static enum test_result pq_test_pop_max(void);
static enum test_result pq_test_pop_min(void);
static enum test_result pq_test_delete_prime_shuffle_duplicates(void);
static enum test_result pq_test_prime_shuffle(void);
static enum test_result pq_test_weak_srand(void);
static enum test_result insert_shuffled(struct pqueue *, struct val[], size_t,
                                        int);
static size_t inorder_fill(int[], size_t, struct pqueue *);
static enum pq_threeway_cmp val_cmp(const struct pq_elem *,
                                    const struct pq_elem *, void *);

#define NUM_TESTS (size_t)9
const test_fn all_tests[NUM_TESTS] = {
    pq_test_insert_remove_four_dups,
    pq_test_insert_erase_shuffled,
    pq_test_pop_max,
    pq_test_pop_min,
    pq_test_delete_prime_shuffle_duplicates,
    pq_test_prime_shuffle,
    pq_test_weak_srand,
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
pq_test_insert_remove_four_dups(void)
{
    struct pqueue ppq = PQ_INIT(PQLES, val_cmp, NULL);
    struct val three_vals[4];
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        pq_push(&ppq, &three_vals[i].elem);
        CHECK(pq_validate(&ppq), true, bool, "%b");
        const size_t size = i + 1;
        CHECK(pq_size(&ppq), size, size_t, "%zu");
    }
    CHECK(pq_size(&ppq), 4, size_t, "%zu");
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        pq_pop(&ppq);
        CHECK(pq_validate(&ppq), true, bool, "%b");
    }
    CHECK(pq_size(&ppq), 0ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_insert_erase_shuffled(void)
{
    struct pqueue ppq = PQ_INIT(PQLES, val_cmp, NULL);
    const size_t size = 50;
    const int prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&ppq, vals, size, prime), PASS, enum test_result,
          "%d");
    const struct val *min = PQ_ENTRY(pq_front(&ppq), struct val, elem);
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &ppq), size, size_t, "%zu");
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i], int, "%d");
    }
    /* Now let's delete everything with no errors. */
    for (size_t i = 0; i < size; ++i)
    {
        (void)pq_erase(&ppq, &vals[i].elem);
        CHECK(pq_validate(&ppq), true, bool, "%b");
    }
    CHECK(pq_size(&ppq), 0ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_pop_max(void)
{
    struct pqueue ppq = PQ_INIT(PQLES, val_cmp, NULL);
    const size_t size = 50;
    const int prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&ppq, vals, size, prime), PASS, enum test_result,
          "%d");
    const struct val *min = PQ_ENTRY(pq_front(&ppq), struct val, elem);
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &ppq), size, size_t, "%zu");
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i], int, "%d");
    }
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = 0; i < size; ++i)
    {
        const struct val *front = PQ_ENTRY(pq_pop(&ppq), struct val, elem);
        CHECK(front->val, vals[i].val, int, "%d");
    }
    CHECK(pq_empty(&ppq), true, bool, "%b");
    return PASS;
}

static enum test_result
pq_test_pop_min(void)
{
    struct pqueue ppq = PQ_INIT(PQLES, val_cmp, NULL);
    const size_t size = 50;
    const int prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&ppq, vals, size, prime), PASS, enum test_result,
          "%d");
    const struct val *min = PQ_ENTRY(pq_front(&ppq), struct val, elem);
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &ppq), size, size_t, "%zu");
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i], int, "%d");
    }
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = 0; i < size; ++i)
    {
        const struct val *front = PQ_ENTRY(pq_pop(&ppq), struct val, elem);
        CHECK(front->val, vals[i].val, int, "%d");
    }
    CHECK(pq_empty(&ppq), true, bool, "%b");
    return PASS;
}

static enum test_result
pq_test_delete_prime_shuffle_duplicates(void)
{
    struct pqueue ppq = PQ_INIT(PQLES, val_cmp, NULL);
    const int size = 99;
    const int prime = 101;
    /* Make the prime shuffle shorter than size for many duplicates. */
    const int less = 77;
    struct val vals[size];
    int shuffled_index = prime % (size - less);
    for (int i = 0; i < size; ++i)
    {
        vals[i].val = shuffled_index;
        vals[i].id = i;
        pq_push(&ppq, &vals[i].elem);
        CHECK(pq_validate(&ppq), true, bool, "%b");
        const size_t s = i + 1;
        CHECK(pq_size(&ppq), s, size_t, "%zu");
        /* Shuffle like this only on insertions to create more dups. */
        shuffled_index = (shuffled_index + prime) % (size - less);
    }

    shuffled_index = prime % (size - less);
    size_t cur_size = size;
    for (int i = 0; i < size; ++i)
    {
        (void)pq_erase(&ppq, &vals[shuffled_index].elem);
        CHECK(pq_validate(&ppq), true, bool, "%b");
        --cur_size;
        CHECK(pq_size(&ppq), cur_size, size_t, "%zu");
        /* Shuffle normally here so we only remove each elem once. */
        shuffled_index = (shuffled_index + prime) % size;
    }
    return PASS;
}

static enum test_result
pq_test_prime_shuffle(void)
{
    struct pqueue ppq = PQ_INIT(PQLES, val_cmp, NULL);
    const int size = 50;
    const int prime = 53;
    const int less = 10;
    /* We want the tree to have a smattering of duplicates so
       reduce the shuffle range so it will repeat some values. */
    int shuffled_index = prime % (size - less);
    struct val vals[size];
    for (int i = 0; i < size; ++i)
    {
        vals[i].val = shuffled_index;
        vals[i].id = shuffled_index;
        pq_push(&ppq, &vals[i].elem);
        CHECK(pq_validate(&ppq), true, bool, "%b");
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    /* Now we go through and free all the elements in order but
       their positions in the tree will be somewhat random */
    size_t cur_size = size;
    for (int i = 0; i < size; ++i)
    {
        CHECK(pq_erase(&ppq, &vals[i].elem) != NULL, true, bool, "%b");
        CHECK(pq_validate(&ppq), true, bool, "%b");
        --cur_size;
        CHECK(pq_size(&ppq), cur_size, size_t, "%zu");
    }
    return PASS;
}

static enum test_result
pq_test_weak_srand(void)
{
    struct pqueue ppq = PQ_INIT(PQLES, val_cmp, NULL);
    /* Seed the test with any integer for reproducible randome test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    const int num_heap_elems = 1000;
    struct val vals[num_heap_elems];
    for (int i = 0; i < num_heap_elems; ++i)
    {
        vals[i].val = rand(); // NOLINT
        vals[i].id = i;
        pq_push(&ppq, &vals[i].elem);
        CHECK(pq_validate(&ppq), true, bool, "%b");
    }
    for (int i = 0; i < num_heap_elems; ++i)
    {
        CHECK(pq_erase(&ppq, &vals[i].elem) != NULL, true, bool, "%b");
        CHECK(pq_validate(&ppq), true, bool, "%b");
    }
    CHECK(pq_empty(&ppq), true, bool, "%b");
    return PASS;
}

static enum test_result
insert_shuffled(struct pqueue *ppq, struct val vals[], const size_t size,
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
        pq_push(ppq, &vals[shuffled_index].elem);
        CHECK(pq_size(ppq), i + 1, size_t, "%zu");
        CHECK(pq_validate(ppq), true, bool, "%b");
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(pq_size(ppq), size, size_t, "%zu");
    return PASS;
}

/* Iterative inorder traversal to check the heap is sorted. */
static size_t
inorder_fill(int vals[], size_t size, struct pqueue *ppq)
{
    if (pq_size(ppq) != size)
    {
        return 0;
    }
    size_t i = 0;
    struct pqueue copy = PQ_INIT(pq_order(ppq), val_cmp, NULL);
    while (!pq_empty(ppq))
    {
        struct pq_elem *const front = pq_pop(ppq);
        CHECK(pq_validate(ppq), true, bool, "%b");
        CHECK(pq_validate(&copy), true, bool, "%b");
        vals[i++] = PQ_ENTRY(front, struct val, elem)->val;
        pq_push(&copy, front);
    }
    while (!pq_empty(&copy))
    {
        pq_push(ppq, pq_pop(&copy));
        CHECK(pq_validate(ppq), true, bool, "%b");
        CHECK(pq_validate(&copy), true, bool, "%b");
    }
    return i;
}

static enum pq_threeway_cmp
val_cmp(const struct pq_elem *a, const struct pq_elem *b, void *aux)
{
    (void)aux;
    struct val *lhs = PQ_ENTRY(a, struct val, elem);
    struct val *rhs = PQ_ENTRY(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}
