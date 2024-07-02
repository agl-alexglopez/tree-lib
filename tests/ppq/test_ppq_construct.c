#include "pair_pqueue.h"
#include "test.h"

struct val
{
    int id;
    int val;
    struct ppq_elem elem;
};

static enum test_result pq_test_empty(void);
static enum ppq_threeway_cmp val_cmp(const struct ppq_elem *,
                                     const struct ppq_elem *, void *);

#define NUM_TESTS (size_t)1
const test_fn all_tests[NUM_TESTS] = {pq_test_empty};

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
pq_test_empty(void)
{
    struct pair_pqueue pq = PPQ_INIT(PPQLES, val_cmp, NULL);
    CHECK(ppq_empty(&pq), true, bool, "%b");
    return PASS;
}

static enum ppq_threeway_cmp
val_cmp(const struct ppq_elem *a, const struct ppq_elem *b, void *aux)
{
    (void)aux;
    struct val *lhs = PPQ_ENTRY(a, struct val, elem);
    struct val *rhs = PPQ_ENTRY(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}
