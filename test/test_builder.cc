#include <gtest/gtest.h>

extern "C" {
#include "builder.h"
}

TEST(builder, crush_create) {
  crush_map *m = crush_create();
  EXPECT_EQ(NULL, m->rules);
  crush_destroy(m);
}

TEST(builder, crush_make_bucket) {
  crush_map *m = crush_create();
  const int type = 1;
  const int size = 1;
  const int hash = 3;
  int weights[1] = { 1 };
  int items[1] = { 1 };
  crush_bucket *b;

  for(auto alg : { CRUSH_BUCKET_UNIFORM, CRUSH_BUCKET_LIST, CRUSH_BUCKET_STRAW2 }) {
    b = crush_make_bucket(m, alg, hash, type, size, items, weights);
    ASSERT_TRUE(b);
    EXPECT_EQ(alg, b->alg);
    crush_destroy_bucket(b);
  }

  crush_destroy(m);
}

TEST(builder, crush_multiplication_is_unsafe) {
  ASSERT_TRUE(crush_multiplication_is_unsafe(1, 0));
}
