#include <errno.h>

#include <gtest/gtest.h>

extern "C" {
#include "crush/hash.h"  
#include "crush/builder.h"
#include "crush/helpers.h"
}

TEST(helpers, crush_find_roots) {
  int *roots = NULL;
  struct crush_map *m = crush_create();
  ASSERT_EQ(crush_find_roots(m, &roots), 0);
  free(roots);
  
  struct crush_bucket *first = crush_make_bucket(m, CRUSH_BUCKET_STRAW2, CRUSH_HASH_DEFAULT, 1,
                                                 0, NULL, NULL);
  int first_bucketno = 0;
  ASSERT_EQ(crush_add_bucket(m, 0, first, &first_bucketno), 0);

  struct crush_bucket *second = crush_make_bucket(m, CRUSH_BUCKET_STRAW2, CRUSH_HASH_DEFAULT, 1,
                                                  0, NULL, NULL);
  int second_bucketno = 0;
  ASSERT_EQ(crush_add_bucket(m, 0, second, &second_bucketno), 0);

  ASSERT_EQ(crush_find_roots(m, &roots), 2);
  ASSERT_EQ(roots[0], first_bucketno);
  ASSERT_EQ(roots[1], second_bucketno);
  free(roots);

  ASSERT_EQ(crush_bucket_add_item(m, first, second_bucketno, 0x1000), 0);
  ASSERT_EQ(crush_find_roots(m, &roots), 1);
  ASSERT_EQ(roots[0], first_bucketno);
  free(roots);

  ASSERT_EQ(crush_bucket_add_item(m, first, -200, 0x1000), 0);
  ASSERT_EQ(crush_find_roots(m, &roots), -EINVAL);

  crush_destroy(m);
}
