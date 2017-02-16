#ifndef CEPH_CRUSH_HELPERS_H
#define CEPH_CRUSH_HELPERS_H

#include "crush.h"

/** @ingroup API
 *
 * Look for all buckets that are not referenced in any buckets and
 * return them in the __buckets__ __malloc(3)__ array. The size of the
 * array is returned. The buckets are negative numbers.
 *
 * It is the responsibility of the caller to __free(3)__ the
 * __buckets__ array allocated by the function. If the function
 * returns on error, the value of the __buckets__ argument is
 * undefined.
 *
 * - return -ENOMEM if __malloc(3)__ fails to allocate the array
 * - return -EINVAL if a bucket references a non existent item
 * 
 * @param[in] map the crush_map
 * @param[out] buckets an array of items with no parents
 * 
 * @returns the size of __buckets__ on success, < 0 on error
 */
extern int crush_find_roots(struct crush_map *map, int **buckets);

#endif
