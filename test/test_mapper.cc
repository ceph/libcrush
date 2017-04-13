#include <gtest/gtest.h>

#include <list>

extern "C" {
#include "hash.h"
#include "builder.h"
#include "mapper.h"
}

TEST(mapper, crush_do_rule_choose_arg) {
  crush_map *m = crush_create();
  const int root_type = 1;
  crush_bucket *root = crush_make_bucket(m, CRUSH_BUCKET_STRAW2, CRUSH_HASH_DEFAULT, root_type,
                                         0, NULL, NULL);
  int rootno = 0;
  ASSERT_EQ(0, crush_add_bucket(m, 0, root, &rootno));
  ASSERT_EQ(-1, rootno);

  int low_weight = 0x10000;
  int high_weight = 0x10000 * 10;
  int host_count = 10;
  int host_type = 2;
  const int b_size = 2;
  for (int host = 0; host < host_count; host++) {
    int bno = 0;
    crush_bucket *b;
    {
      int weights[b_size];
      int items[b_size];
      for (int i = 0; i < b_size; i++) {
        if (i == 0)
          weights[i] = low_weight;
        else
          weights[i] = high_weight;
        items[i] = host * b_size + i;
        printf("host %d item %d weight 0x%x\n", host, items[i], weights[i]);
      }
      b = crush_make_bucket(m, CRUSH_BUCKET_STRAW2, CRUSH_HASH_DEFAULT, host_type, b_size, items, weights);
      ASSERT_EQ(0, crush_add_bucket(m, 0, b, &bno));
      printf("host %d add bucket %d weight 0x%x\n", host, bno, b->weight);
      ASSERT_EQ(0, crush_bucket_add_item(m, root, bno, b->weight));
    }
  }

  ASSERT_EQ(host_count, root->size);

  crush_finalize(m);

  int device_count = host_count * b_size;
  int weights_size = device_count;
  __u32 weights[weights_size];
  for (int weight = 0; weight < weights_size; weight++)
    weights[weight] = low_weight;

  int replication_count = 2;

  int num_positions = replication_count;

  int result[replication_count];
  memset(result, '\0', sizeof(int) * replication_count);
  int cwin_size = crush_work_size(m, replication_count);
  char cwin[cwin_size];
  crush_init_workspace(m, cwin);

  int steps_count = 3;

  std::vector<int> ruleno_list;

  struct crush_rule *rule = crush_make_rule(steps_count, 0, 0, 0, 0);
  ASSERT_EQ(steps_count, rule->len);
  crush_rule_set_step(rule, 0, CRUSH_RULE_TAKE, rootno, 0);
  crush_rule_set_step(rule, 1, CRUSH_RULE_CHOOSELEAF_FIRSTN, 0, host_type);
  crush_rule_set_step(rule, 2, CRUSH_RULE_EMIT, 0, 0);
  ruleno_list.push_back(crush_add_rule(m, rule, -1));

  rule = crush_make_rule(steps_count, 0, 0, 0, 0);
  ASSERT_EQ(steps_count, rule->len);
  crush_rule_set_step(rule, 0, CRUSH_RULE_TAKE, rootno, 0);
  crush_rule_set_step(rule, 1, CRUSH_RULE_CHOOSELEAF_INDEP, 0, host_type);
  crush_rule_set_step(rule, 2, CRUSH_RULE_EMIT, 0, 0);
  ruleno_list.push_back(crush_add_rule(m, rule, -1));

  int value = 1234;
  int result_len;
  for (auto ruleno : ruleno_list) {
    struct crush_choose_arg *choose_args = crush_make_choose_args(m, num_positions);

    {
      int host_9 = -11;
      int device_18 = 0;
      int device_19 = 1;
      __u32 *position_0 = choose_args[-1-host_9].weight_set[0].weights;
      ASSERT_EQ(low_weight, position_0[device_18]);
      ASSERT_EQ(high_weight, position_0[device_19]);
      __u32 *position_1 = choose_args[-1-host_9].weight_set[1].weights;
      ASSERT_EQ(low_weight, position_1[device_18]);
      ASSERT_EQ(high_weight, position_1[device_19]);

      //
      // 1234 maps to [19, 13]
      //
      result_len = crush_do_rule(m,
                                 ruleno,
                                 value,
                                 result, replication_count,
                                 weights, weights_size,
                                 cwin,
                                 choose_args);
      //
      // NULL choose_args leads to the same result as the default
      //
      result_len = crush_do_rule(m,
                                 ruleno,
                                 value,
                                 result, replication_count,
                                 weights, weights_size,
                                 cwin,
                                 NULL);

      ASSERT_EQ(replication_count, result_len);
      ASSERT_EQ(19, result[0]);
      ASSERT_EQ(13, result[1]);

      //
      // swap device_18 and device_19 weights in first position so that
      // 1234 maps to [18, 13] instead
      //
      std::swap(position_0[device_18], position_0[device_19]);

      result_len = crush_do_rule(m,
                                 ruleno,
                                 value,
                                 result, replication_count,
                                 weights, weights_size,
                                 cwin,
                                 choose_args);

      ASSERT_EQ(replication_count, result_len);
      ASSERT_EQ(18, result[0]);
      ASSERT_EQ(13, result[1]);
    }

    {
      int host_6 = -8;
      int device_12 = 0;
      int device_13 = 1;
      __u32 *position_0 = choose_args[-1-host_6].weight_set[0].weights;
      ASSERT_EQ(low_weight, position_0[device_12]);
      ASSERT_EQ(high_weight, position_0[device_13]);
      __u32 *position_1 = choose_args[-1-host_6].weight_set[1].weights;
      ASSERT_EQ(low_weight, position_1[device_12]);
      ASSERT_EQ(high_weight, position_1[device_13]);
      //
      // swap device_12 and device_13 weights in first position does
      // not change the result because they are in second position
      //
      std::swap(position_0[device_12], position_0[device_13]);

      result_len = crush_do_rule(m,
                                 ruleno,
                                 value,
                                 result, replication_count,
                                 weights, weights_size,
                                 cwin,
                                 choose_args);

      ASSERT_EQ(replication_count, result_len);
      ASSERT_EQ(18, result[0]);
      ASSERT_EQ(13, result[1]);

      //
      // The weights in first position are used for second
      // position as well when there are no weights for
      // second position
      //
      ASSERT_EQ(2, choose_args[-1-host_6].weight_set_size);
      choose_args[-1-host_6].weight_set_size = 1;

      result_len = crush_do_rule(m,
                                 ruleno,
                                 value,
                                 result, replication_count,
                                 weights, weights_size,
                                 cwin,
                                 choose_args);

      ASSERT_EQ(replication_count, result_len);
      ASSERT_EQ(18, result[0]);
      ASSERT_EQ(12, result[1]);
      choose_args[-1-host_6].weight_set_size = 2;

      //
      // swap device_12 and device_13 weights in second position so that
      // 1234 maps to [18, 12] instead
      //
      std::swap(position_1[device_12], position_1[device_13]);

      result_len = crush_do_rule(m,
                                 ruleno,
                                 value,
                                 result, replication_count,
                                 weights, weights_size,
                                 cwin,
                                 choose_args);

      ASSERT_EQ(replication_count, result_len);
      ASSERT_EQ(18, result[0]);
      ASSERT_EQ(12, result[1]);
    }

    {
      int host_6 = -8;
      //
      // Verify that a NULL weight_set / ids does not crash
      //
      choose_args[-1-host_6].weight_set = NULL;
      choose_args[-1-host_6].weight_set_size = 0;
      choose_args[-1-host_6].ids = NULL;
      choose_args[-1-host_6].ids_size = 0;
      result_len = crush_do_rule(m,
                                 ruleno,
                                 value,
                                 result, replication_count,
                                 weights, weights_size,
                                 cwin,
                                 choose_args);
      ASSERT_EQ(replication_count, result_len);
      ASSERT_EQ(18, result[0]);
      ASSERT_EQ(13, result[1]);
    }

    crush_destroy_choose_args(choose_args);
  }

  {
    struct crush_choose_arg *choose_args = crush_make_choose_args(m, num_positions);
    {
      int host_6 = -8;
      int device_12 = 0;
      int device_13 = 1;
      __u32 *position_0 = choose_args[-1-host_6].weight_set[0].weights;
      ASSERT_EQ(low_weight, position_0[device_12]);
      ASSERT_EQ(high_weight, position_0[device_13]);
      __u32 *position_1 = choose_args[-1-host_6].weight_set[1].weights;
      ASSERT_EQ(low_weight, position_1[device_12]);
      ASSERT_EQ(high_weight, position_1[device_13]);

      //
      // set the weight of all devices to the same value
      //
      choose_args[-1-host_6].weight_set_size = 1;
      position_0[device_12] = position_0[device_13] = 0x10000;
      result_len = crush_do_rule(m,
                                 ruleno_list[0],
                                 value,
                                 result, replication_count,
                                 weights, weights_size,
                                 cwin,
                                 choose_args);

      ASSERT_EQ(replication_count, result_len);
      ASSERT_EQ(19, result[0]);
      ASSERT_EQ(12, result[1]);

      //
      // modify the id remapping and verify this has a side
      // effect on mapping
      //
      ASSERT_EQ(12, choose_args[-1-host_6].ids[device_12]);
      choose_args[-1-host_6].ids[device_12] = 200;
      result_len = crush_do_rule(m,
                                 ruleno_list[0],
                                 value,
                                 result, replication_count,
                                 weights, weights_size,
                                 cwin,
                                 choose_args);

      ASSERT_EQ(replication_count, result_len);
      ASSERT_EQ(19, result[0]);
      ASSERT_EQ(13, result[1]);

      choose_args[-1-host_6].weight_set_size = 2;
      position_0[device_12] = low_weight;
      position_0[device_13] = high_weight;
    }
    crush_destroy_choose_args(choose_args);
  }

  crush_destroy(m);
}

// Local Variables:
// compile-command: "cd ../build ; make unittest_mapper && valgrind --tool=memcheck test/unittest_mapper"
// End:
