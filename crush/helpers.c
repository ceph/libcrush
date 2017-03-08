#include <errno.h>

#include "helpers.h"

int crush_find_roots(struct crush_map *map, int **buckets)
{
  int ref[map->max_buckets];
  int root_count = map->max_buckets;
  int pos, i;

  memset(ref, '\0', sizeof(ref));
  for (pos = 0; pos < map->max_buckets; pos++) {
    struct crush_bucket *b = map->buckets[pos];
    if (b == NULL) {
      root_count--;
      continue;
    }
    for (i = 0; i < b->size; i++) {
      if (b->items[i] >= 0)
        continue;
      int item = -1-b->items[i];
      if (item >= map->max_buckets)
        return -EINVAL;
      if (ref[item] == 0)
        root_count--;
      ref[item]++;
    }
  }

  int *roots = (int*)malloc(root_count * sizeof(int));
  if (roots == NULL)
    return -ENOMEM;
  int roots_length = 0;
  for (pos = 0; pos < map->max_buckets; pos++) {
    if (map->buckets[pos] != NULL && ref[pos] == 0)
      roots[roots_length++] = -1-pos;
  }
  assert(roots_length == root_count);
  *buckets = roots;
  return root_count;
}
