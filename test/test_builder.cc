#include <gtest/gtest.h>

extern "C" {
#include "builder.h"
}

TEST(builder, crush_create) {
  crush_map *c = crush_create();
  EXPECT_EQ(NULL, c->rules);
  free(c);
}
