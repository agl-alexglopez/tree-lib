#include "set.h"
#include "test.h"
#include "tree.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

struct val
{
    int id;
    int val;
    struct set_elem elem;
};

static enum test_result set_test_forward_iter(void);
static enum test_result set_test_iterate_removal(void);
static enum test_result set_test_iterate_remove_reinsert(void);
static enum test_result set_test_valid_range(void);
static enum test_result set_test_invalid_range(void);
static enum test_result set_test_empty_range(void);
static size_t inorder_fill(int[], size_t, struct set *);
static enum test_result iterator_check(struct set *);
static set_threeway_cmp val_cmp(struct set_elem const *,
                                struct set_elem const *, void *);

#define NUM_TESTS ((size_t)6)
test_fn const all_tests[NUM_TESTS] = {
    set_test_forward_iter, set_test_iterate_removal,
    set_test_valid_range,  set_test_invalid_range,
    set_test_empty_range,  set_test_iterate_remove_reinsert,
};

int
main()
{
    enum test_result res = PASS;
    for (size_t i = 0; i < NUM_TESTS; ++i)
    {
        bool const fail = all_tests[i]() == FAIL;
        if (fail)
        {
            res = FAIL;
        }
    }
    return res;
}

static enum test_result
set_test_forward_iter(void)
{
    struct set s = SET_INIT(s, val_cmp, NULL);
    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (struct set_elem *e = set_begin(&s); e != set_end(&s);
         e = set_next(&s, e), ++j)
    {}
    CHECK(j, 0, int, "%d");
    int const num_nodes = 33;
    int const prime = 37;
    struct val vals[num_nodes];
    size_t shuffled_index = prime % num_nodes;
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].val = (int)shuffled_index;
        vals[i].id = i;
        set_insert(&s, &vals[i].elem);
        CHECK(validate_tree(&s.t), true, bool, "%d");
        shuffled_index = (shuffled_index + prime) % num_nodes;
    }
    int val_keys_inorder[num_nodes];
    CHECK(inorder_fill(val_keys_inorder, num_nodes, &s), set_size(&s), size_t,
          "%zu");
    j = 0;
    for (struct set_elem *e = set_begin(&s); e != set_end(&s) && j < num_nodes;
         e = set_next(&s, e), ++j)
    {
        struct val const *v = SET_ENTRY(e, struct val, elem);
        CHECK(v->val, val_keys_inorder[j], int, "%d");
    }
    return PASS;
}

static enum test_result
set_test_iterate_removal(void)
{
    struct set s = SET_INIT(s, val_cmp, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct val vals[num_nodes];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        set_insert(&s, &vals[i].elem);
        CHECK(validate_tree(&s.t), true, bool, "%d");
    }
    CHECK(iterator_check(&s), PASS, enum test_result, "%d");
    int const limit = 400;
    for (struct set_elem *i = set_begin(&s), *next = NULL; i != set_end(&s);
         i = next)
    {
        next = set_next(&s, i);
        struct val *cur = SET_ENTRY(i, struct val, elem);
        if (cur->val > limit)
        {
            (void)set_erase(&s, i);
            CHECK(validate_tree(&s.t), true, bool, "%d");
        }
    }
    return PASS;
}

static enum test_result
set_test_iterate_remove_reinsert(void)
{
    struct set s = SET_INIT(s, val_cmp, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    size_t const num_nodes = 1000;
    struct val vals[num_nodes];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        set_insert(&s, &vals[i].elem);
        CHECK(validate_tree(&s.t), true, bool, "%d");
    }
    CHECK(iterator_check(&s), PASS, enum test_result, "%d");
    size_t const old_size = set_size(&s);
    int const limit = 400;
    int new_unique_entry_val = 1001;
    for (struct set_elem *i = set_begin(&s), *next = NULL; i != set_end(&s);
         i = next)
    {
        next = set_next(&s, i);
        struct val *cur = SET_ENTRY(i, struct val, elem);
        if (cur->val < limit)
        {
            (void)set_erase(&s, i);
            struct val *v = SET_ENTRY(i, struct val, elem);
            v->val = new_unique_entry_val;
            CHECK(set_insert(&s, i), true, bool, "%d");
            CHECK(validate_tree(&s.t), true, bool, "%d");
            ++new_unique_entry_val;
        }
    }
    CHECK(set_size(&s), old_size, size_t, "%zu");
    return PASS;
}

static enum test_result
set_test_valid_range(void)
{
    struct set s = SET_INIT(s, val_cmp, NULL);

    int const num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        set_insert(&s, &vals[i].elem);
        CHECK(validate_tree(&s.t), true, bool, "%d");
    }
    struct val b = {.id = 0, .val = 6};
    struct val e = {.id = 0, .val = 44};
    /* This should be the following range [6,44). 6 should raise to
       next value not less than 6, 10 and 44 should be the first
       value greater than 44, 45. */
    int const range_vals[8] = {10, 15, 20, 25, 30, 35, 40, 45};
    struct set_range const range = set_equal_range(&s, &b.elem, &e.elem);
    CHECK(SET_ENTRY(set_begin_range(&range), struct val, elem)->val,
          range_vals[0], int, "%d");
    CHECK(SET_ENTRY(set_end_range(&range), struct val, elem)->val,
          range_vals[7], int, "%d");
    size_t index = 0;
    struct set_elem *i1 = set_begin_range(&range);
    for (; i1 != set_end_range(&range); i1 = set_next(&s, i1))
    {
        int const cur_val = SET_ENTRY(i1, struct val, elem)->val;
        CHECK(range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i1, set_end_range(&range), struct set_elem *, "%p");
    CHECK(SET_ENTRY(i1, struct val, elem)->val, range_vals[7], int, "%d");
    b.val = 119;
    e.val = 84;
    /* This should be the following range [119,84). 119 should be
       dropped to first value not greater than 119 and last should
       be dropped to first value less than 84. */
    int const rev_range_vals[8] = {115, 110, 105, 100, 95, 90, 85, 80};
    struct set_rrange const rev_range = set_equal_rrange(&s, &b.elem, &e.elem);
    CHECK(SET_ENTRY(set_begin_rrange(&rev_range), struct val, elem)->val,
          rev_range_vals[0], int, "%d");
    CHECK(SET_ENTRY(set_end_rrange(&rev_range), struct val, elem)->val,
          rev_range_vals[7], int, "%d");
    index = 0;
    struct set_elem *i2 = set_begin_rrange(&rev_range);
    for (; i2 != set_end_rrange(&rev_range); i2 = set_rnext(&s, i2))
    {
        int const cur_val = SET_ENTRY(i2, struct val, elem)->val;
        CHECK(rev_range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i2, set_end_rrange(&rev_range), struct set_elem *, "%p");
    CHECK(SET_ENTRY(i2, struct val, elem)->val, rev_range_vals[7], int, "%d");
    return PASS;
}

static enum test_result
set_test_invalid_range(void)
{
    struct set s = SET_INIT(s, val_cmp, NULL);

    int const num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        set_insert(&s, &vals[i].elem);
        CHECK(validate_tree(&s.t), true, bool, "%d");
    }
    struct val b = {.id = 0, .val = 95};
    struct val e = {.id = 0, .val = 999};
    /* This should be the following range [95,999). 95 should raise to
       next value not less than 95, 95 and 999 should be the first
       value greater than 999, none or the end. */
    int const forward_range_vals[6] = {95, 100, 105, 110, 115, 120};
    struct set_range const rev_range = set_equal_range(&s, &b.elem, &e.elem);
    CHECK(SET_ENTRY(set_begin_range(&rev_range), struct val, elem)->val
              == forward_range_vals[0],
          true, bool, "%d");
    CHECK(set_end_range(&rev_range), set_end(&s), struct set_elem *, "%p");
    size_t index = 0;
    struct set_elem *i1 = set_begin_range(&rev_range);
    for (; i1 != set_end_range(&rev_range); i1 = set_next(&s, i1))
    {
        int const cur_val = SET_ENTRY(i1, struct val, elem)->val;
        CHECK(forward_range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i1, set_end_range(&rev_range), struct set_elem *, "%p");
    CHECK(i1, set_end(&s), struct set_elem *, "%p");
    b.val = 36;
    e.val = -999;
    /* This should be the following range [36,-999). 36 should be
       dropped to first value not greater than 36 and last should
       be dropped to first value less than -999 which is end. */
    int const rev_range_vals[8] = {35, 30, 25, 20, 15, 10, 5, 0};
    struct set_rrange const range = set_equal_rrange(&s, &b.elem, &e.elem);
    CHECK(SET_ENTRY(set_begin_rrange(&range), struct val, elem)->val,
          rev_range_vals[0], int, "%d");
    CHECK(set_end_rrange(&range), set_end(&s), struct set_elem *, "%p");
    index = 0;
    struct set_elem *i2 = set_begin_rrange(&range);
    for (; i2 != set_end_rrange(&range); i2 = set_rnext(&s, i2))
    {
        int const cur_val = SET_ENTRY(i2, struct val, elem)->val;
        CHECK(rev_range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i2, set_end_rrange(&range), struct set_elem *, "%p");
    CHECK(i2, set_end(&s), struct set_elem *, "%p");
    return PASS;
}

static enum test_result
set_test_empty_range(void)
{
    struct set s = SET_INIT(s, val_cmp, NULL);

    int const num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        set_insert(&s, &vals[i].elem);
        CHECK(validate_tree(&s.t), true, bool, "%d");
    }
    /* Nonexistant range returns end [begin, end) in both positions.
       which may not be the end element but a value in the tree. However,
       Normal iteration patterns would consider this empty. */
    struct val b = {.id = 0, .val = -50};
    struct val e = {.id = 0, .val = -25};
    struct set_range const forward_range
        = set_equal_range(&s, &b.elem, &e.elem);
    CHECK(SET_ENTRY(set_begin_range(&forward_range), struct val, elem)->val,
          vals[0].val, int, "%d");
    CHECK(SET_ENTRY(set_end_range(&forward_range), struct val, elem)->val,
          vals[0].val, int, "%d");
    b.val = 150;
    e.val = 999;
    struct set_rrange const rev_range = set_equal_rrange(&s, &b.elem, &e.elem);
    CHECK(SET_ENTRY(set_begin_rrange(&rev_range), struct val, elem)->val,
          vals[num_nodes - 1].val, int, "%d");
    CHECK(SET_ENTRY(set_end_rrange(&rev_range), struct val, elem)->val,
          vals[num_nodes - 1].val, int, "%d");
    return PASS;
}

/* Iterative inorder traversal to check the heap is sorted. */
static size_t
inorder_fill(int vals[], size_t size, struct set *s)
{
    if (set_size(s) != size)
    {
        return 0;
    }
    size_t i = 0;
    for (struct set_elem *e = set_begin(s); e != set_end(s); e = set_next(s, e))
    {
        vals[i++] = SET_ENTRY(e, struct val, elem)->val;
    }
    return i;
}

static enum test_result
iterator_check(struct set *s)
{
    size_t const size = set_size(s);
    size_t iter_count = 0;
    for (struct set_elem *e = set_begin(s); e != set_end(s); e = set_next(s, e))
    {
        ++iter_count;
        CHECK(iter_count != size || set_is_max(s, e), true, bool, "%d");
        CHECK(iter_count == size || !set_is_max(s, e), true, bool, "%d");
    }
    CHECK(iter_count, size, size_t, "%zu");
    iter_count = 0;
    for (struct set_elem *e = set_rbegin(s); e != set_end(s);
         e = set_rnext(s, e))
    {
        ++iter_count;
        CHECK(iter_count != size || set_is_min(s, e), true, bool, "%d");
        CHECK(iter_count == size || !set_is_min(s, e), true, bool, "%d");
    }
    CHECK(iter_count, size, size_t, "%zu");
    return PASS;
}

static set_threeway_cmp
val_cmp(struct set_elem const *a, struct set_elem const *b, void *aux)
{
    (void)aux;
    struct val *lhs = SET_ENTRY(a, struct val, elem);
    struct val *rhs = SET_ENTRY(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}
