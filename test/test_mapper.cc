#include <gtest/gtest.h>

#include <list>

extern "C" {
#include "hash.h"
#include "builder.h"
#include "mapper.h"
}

TEST(mapper, crush_do_rule_weights) {
  crush_map *m = crush_create();
  const int type = 1;
  const int size = 1;
  const int hash = 3;
  int weights[1] = { 110 };
  int items[1] = { 1 };
  crush_bucket *b = crush_make_bucket(m, CRUSH_BUCKET_UNIFORM, hash, type,
                                      size, items, weights);
  EXPECT_EQ(CRUSH_BUCKET_UNIFORM, b->alg);
  int rootno = 0;
  ASSERT_EQ(0, crush_add_bucket(m, 0, b, &rootno));
  crush_destroy(m);
}

// Local Variables:
// compile-command: "cd ../build ; make unittest_mapper && valgrind --tool=memcheck test/unittest_mapper"
// End:
