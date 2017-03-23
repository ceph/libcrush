#include <gtest/gtest.h>

extern "C" {
#include "crush/builder.h"
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

TEST(builder, crush_add_bucket) {
  crush_map *m = crush_create();
  const int type = 1;
  const int size = 1;
  const int hash = 3;
  crush_bucket *b = crush_make_bucket(m, CRUSH_BUCKET_STRAW2, hash, type,
                                      0, NULL, NULL);
  int bucketno = 0;
  ASSERT_EQ(0, crush_add_bucket(m, 0, b, &bucketno));
  ASSERT_EQ(-1, bucketno);
  ASSERT_EQ(-EEXIST, crush_add_bucket(m, bucketno, b, &bucketno));

  for(int i = 0; i < 1024; i++) {
    crush_bucket *b = crush_make_bucket(m, CRUSH_BUCKET_STRAW2, hash, type,
                                        0, NULL, NULL);
    ASSERT_EQ(0, m->max_buckets % 2);
    ASSERT_EQ(0, crush_add_bucket(m, 0, b, &bucketno));
  }
  crush_destroy(m);
}

TEST(builder, crush_multiplication_is_unsafe) {
  ASSERT_TRUE(crush_multiplication_is_unsafe(1, 0));
}

TEST(builder, crush_bucket__add_item_uniform) {
  crush_map *m = crush_create();
  const int type = 1;
  const int size = 1;
  const int hash = 3;
  crush_bucket *b = crush_make_bucket(m, CRUSH_BUCKET_UNIFORM, hash, type,
                                      0, NULL, NULL);
  int bucketno = 0;
  ASSERT_EQ(0, crush_add_bucket(m, 0, b, &bucketno));
  /* For a kind CRUSH_BUCKET_UNIFORM, if no item weights has been
     passed to 'crush_make_bucket', by default 0 is used. */
  ASSERT_EQ(0, crush_bucket_add_item(m, b, 0, 0));
  ASSERT_EQ(-EINVAL, crush_bucket_add_item(m, b, 0, 1));
  crush_destroy(m);
}
