#include <gtest/gtest.h>

extern "C" {
#include "crush/builder.h"
}

TEST(builder, crush_create) {
  crush_map *m = crush_create();
  EXPECT_EQ(NULL, m->rules);

  // Ensures that the map is well configured
  EXPECT_EQ(0, m->choose_local_tries);
  EXPECT_EQ(0, m->choose_local_fallback_tries);
  EXPECT_EQ(50, m->choose_total_tries);
  EXPECT_EQ(1, m->chooseleaf_descend_once);
  EXPECT_EQ(1, m->chooseleaf_vary_r);
  EXPECT_EQ(1, m->chooseleaf_stable);
  EXPECT_EQ(0, m->straw_calc_version);
  EXPECT_EQ((1 << CRUSH_BUCKET_UNIFORM) |
	    (1 << CRUSH_BUCKET_LIST) |
	    (1 << CRUSH_BUCKET_STRAW) |
	    (1 << CRUSH_BUCKET_STRAW2), m->allowed_bucket_algs);

  crush_destroy(m);
}

TEST(builder, crush_create_legacy) {
  crush_map *m = crush_create();
  EXPECT_EQ(NULL, m->rules);

  set_legacy_crush_map(m);
  // Ensures that the map is well configured
  EXPECT_EQ(2, m->choose_local_tries);
  EXPECT_EQ(5, m->choose_local_fallback_tries);
  EXPECT_EQ(19, m->choose_total_tries);
  EXPECT_EQ(0, m->chooseleaf_descend_once);
  EXPECT_EQ(0, m->chooseleaf_vary_r);
  EXPECT_EQ(0, m->chooseleaf_stable);
  EXPECT_EQ(0, m->straw_calc_version);
  EXPECT_EQ(CRUSH_LEGACY_ALLOWED_BUCKET_ALGS, m->allowed_bucket_algs);

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

TEST(builder, crush_bucket_add_item_uniform) {
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

TEST(builder, crush_make_choose_args) {
  crush_map *m = crush_create();
  const int type = 1;
  const int hash = 3;
  crush_bucket *root = crush_make_bucket(m, CRUSH_BUCKET_STRAW2, hash, type,
                                         0, NULL, NULL);
  int rootno = 0;
  ASSERT_EQ(0, crush_add_bucket(m, 0, root, &rootno));
  ASSERT_EQ(-1, rootno);

  const int b1_size = 2;
  int b1no = 0;
  crush_bucket *b1;
  int b1_weight_0 = 10;
  int b1_weight_1 = 20;
  int b1_item_0 = 1;
  int b1_item_1 = 2;
  {
    int weights[2] = { b1_weight_0, b1_weight_1 };
    int items[2] = { b1_item_0, b1_item_1 };

    b1 = crush_make_bucket(m, CRUSH_BUCKET_STRAW2, hash, type, b1_size, items, weights);
    ASSERT_EQ(0, crush_add_bucket(m, 0, b1, &b1no));
    ASSERT_EQ(0, crush_bucket_add_item(m, root, b1no, b1->weight));
  }

  const int b2_size = 1;
  int b2no = 0;
  crush_bucket *b2;
  int b2_weight_0 = 30;
  int b2_item_0 = 3;
  {
    int weights[1] = { b2_weight_0 };
    int items[1] = { b2_item_0 };

    b2 = crush_make_bucket(m, CRUSH_BUCKET_STRAW2, hash, type, b2_size, items, weights);
    ASSERT_EQ(0, crush_add_bucket(m, 0, b2, &b2no));
    ASSERT_EQ(0, crush_bucket_add_item(m, root, b2no, b2->weight));
  }

  ASSERT_EQ(2, root->size);

  int num_positions = 2;
  crush_choose_arg *choose_args = crush_make_choose_args(m, 2);
  for (int position = 0; position < num_positions; ++position) {
    ASSERT_EQ(b1_weight_0, choose_args[-1-b1no].weight_set[position].weights[0]);
    ASSERT_EQ(b1_weight_1, choose_args[-1-b1no].weight_set[position].weights[1]);
    ASSERT_EQ(2, choose_args[-1-b1no].weight_set[position].size);
    ASSERT_EQ(2, choose_args[-1-b1no].weight_set_size);
    ASSERT_EQ(b2_weight_0, choose_args[-1-b2no].weight_set[position].weights[0]);
    ASSERT_EQ(1, choose_args[-1-b2no].weight_set[position].size);
    ASSERT_EQ(2, choose_args[-1-b2no].weight_set_size);
  }
  ASSERT_EQ(b1_item_0, choose_args[-1-b1no].ids[0]);
  ASSERT_EQ(b1_item_1, choose_args[-1-b1no].ids[1]);
  ASSERT_EQ(2, choose_args[-1-b1no].ids_size);
  ASSERT_EQ(b2_item_0, choose_args[-1-b2no].ids[0]);
  ASSERT_EQ(1, choose_args[-1-b2no].ids_size);

  crush_destroy_choose_args(choose_args);
  crush_destroy(m);
}

TEST(builder, crush_make_rule) {
  int ruleset = 0;
  int steps_count = 1;
  int rule_type = 0;
  int minsize = 1;
  int maxsize = 2;
  struct crush_rule *rule;
  rule = crush_make_rule(steps_count, ruleset, rule_type, minsize, maxsize);
  EXPECT_EQ(steps_count, rule->len);
  crush_destroy_rule(rule);
}

// Local Variables:
// compile-command: "cd ../build ; make unittest_builder && valgrind --tool=memcheck test/unittest_builder"
// End:
