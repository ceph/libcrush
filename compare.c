#define NUMBER_OF_OBJECTS 100000

#include "mapper.h"
#include "builder.h"
#include "crush.h"
#include "hash.h"
#include "stdio.h"

#include <time.h>
#include <stdlib.h>
#include <stddef.h>
#include "rbtree.h"

/* 64 bit FNV-1a non-zero initial basis */
#define FNV1A_64_INIT ((uint64_t) 0xcbf29ce484222325ULL)
#define FNV_64_PRIME ((uint64_t) 0x100000001b3ULL)

/* 64 bit Fowler/Noll/Vo FNV-1a hash code */
static inline uint64_t fnv_64a_buf(const void *buf, size_t len, uint64_t hval)
{
	const unsigned char *p = (const unsigned char *) buf;
	int i;

	for (i = 0; i < len; i++) {
		hval ^= (uint64_t) p[i];
		hval *= FNV_64_PRIME;
	}

	return hval;
}

/*
 * The result is same as fnv_64a_buf(&oid, sizeof(oid), hval) but this function
 * is a bit faster.
 */
static inline uint64_t fnv_64a_64(uint64_t oid, uint64_t hval)
{
	hval ^= oid & 0xff;
	hval *= FNV_64_PRIME;
	hval ^= oid >> 8 & 0xff;
	hval *= FNV_64_PRIME;
	hval ^= oid >> 16 & 0xff;
	hval *= FNV_64_PRIME;
	hval ^= oid >> 24 & 0xff;
	hval *= FNV_64_PRIME;
	hval ^= oid >> 32 & 0xff;
	hval *= FNV_64_PRIME;
	hval ^= oid >> 40 & 0xff;
	hval *= FNV_64_PRIME;
	hval ^= oid >> 48 & 0xff;
	hval *= FNV_64_PRIME;
	hval ^= oid >> 56 & 0xff;
	hval *= FNV_64_PRIME;

	return hval;
}

static inline uint64_t sd_hash(const void *buf, size_t len)
{
	uint64_t hval = fnv_64a_buf(buf, len, FNV1A_64_INIT);

	return fnv_64a_64(hval, hval);
}

static inline uint64_t sd_hash_64(uint64_t oid)
{
	uint64_t hval = fnv_64a_64(oid, FNV1A_64_INIT);

	return fnv_64a_64(hval, hval);
}

static inline uint64_t sd_hash_next(uint64_t hval)
{
	return fnv_64a_64(hval, hval);
}

/*
 * Create a hash value from an object id.  The result is same as sd_hash(&oid,
 * sizeof(oid)) but this function is a bit faster.
 */
static inline uint64_t sd_hash_oid(uint64_t oid)
{
	return sd_hash_64(oid);
}

#ifndef __KERNEL__
static inline uint64_t hash_64(uint64_t val, unsigned int bits)
{
	return sd_hash_64(val) >> (64 - bits);
}
#endif

struct node_id {
	uint8_t addr[16];
	uint16_t port;
	uint8_t io_addr[16];
	uint16_t io_port;
#ifndef HAVE_ACCELIO
	uint8_t pad[4];
#else
	uint8_t io_transport_type;
	uint8_t pad[3];
#endif
};

struct sd_node {
	struct rb_node  rb;
	struct node_id  nid;
	uint16_t	nr_vnodes;
	uint32_t	zone;
	uint64_t        space;
#ifdef HAVE_DISKVNODES
	#define SD_MAX_NODES 830
	#define SD_NODE_SIZE (80 + sizeof(struct disk_info) * DISK_MAX)
	struct disk_info disks[DISK_MAX];
#else
	#define SD_MAX_NODES 6144
	#define SD_NODE_SIZE 80
  //	struct disk_info disks[0];
#endif
};

struct sd_vnode {
	struct rb_node rb;
  	struct sd_node *node;
	uint64_t hash;
};

static inline int same_zone(const struct sd_vnode *v1,
			    const struct sd_vnode *v2)
{
	return v1->node->zone == v2->node->zone;
}

/*
 * Compares two integer values
 *
 * If the first argument is larger than the second one, intcmp() returns 1.  If
 * two members are equal, returns 0.  Otherwise, returns -1.
 */
#define intcmp(x, y) \
({					\
	typeof(x) _x = (x);		\
	typeof(y) _y = (y);		\
	(void) (&_x == &_y);		\
	_x < _y ? -1 : _x > _y ? 1 : 0;	\
})

static inline int node_id_cmp(const struct node_id *node1,
			      const struct node_id *node2)
{
	int cmp = memcmp(node1->addr, node2->addr, sizeof(node1->addr));
	if (cmp != 0)
		return cmp;

	return intcmp(node1->port, node2->port);
}

static inline int node_cmp(const struct sd_node *node1,
			   const struct sd_node *node2)
{
	return node_id_cmp(&node1->nid, &node2->nid);
}

static inline int vnode_cmp(const struct sd_vnode *node1,
			    const struct sd_vnode *node2)
{
	return intcmp(node1->hash, node2->hash);
}

#if 0 // for the record, not used but copy/pasted
static inline void
node_to_vnodes(const struct sd_node *n, struct rb_root *vroot)
{
	uint64_t hval = sd_hash(&n->nid, offsetof(typeof(n->nid),
						  io_addr));

	for (int i = 0; i < n->nr_vnodes; i++) {
		struct sd_vnode *v = xmalloc(sizeof(*v));

		hval = sd_hash_next(hval);
		v->hash = hval;
		v->node = n;
		if (unlikely(rb_insert(vroot, v, rb, vnode_cmp)))
			panic("vdisk hash collison");
	}
}
#endif

/* If v1_hash < oid_hash <= v2_hash, then oid is resident on v2 */
static inline struct sd_vnode *
oid_to_first_vnode(uint64_t oid, struct rb_root *root)
{
	struct sd_vnode dummy = {
		.hash = sd_hash_oid(oid),
	};
	return rb_nsearch(root, &dummy, rb, vnode_cmp);
}

#define unlikely(x)     __builtin_expect(!!(x), 0)
#define panic(x) abort()

/* Replica are placed along the ring one by one with different zones */
static inline void oid_to_vnodes(uint64_t oid, struct rb_root *root,
				 int nr_copies,
				 const struct sd_vnode **vnodes)
{
	const struct sd_vnode *next = oid_to_first_vnode(oid, root);

	vnodes[0] = next;
	for (int i = 1; i < nr_copies; i++) {
next:
		next = rb_entry(rb_next(&next->rb), struct sd_vnode, rb);
		if (!next) /* Wrap around */
			next = rb_entry(rb_first(root), struct sd_vnode, rb);
		if (unlikely(next == vnodes[0]))
			panic("can't find a valid vnode");
		for (int j = 0; j < i; j++)
			if (same_zone(vnodes[j], next))
				goto next;
		vnodes[i] = next;
	}
}

struct sd_vnode *allocate_vnode() {
  struct sd_vnode *vnode = (struct sd_vnode*)malloc(sizeof(struct sd_vnode));
  vnode->node = (struct sd_node*)malloc(sizeof(struct sd_node));
}

void deallocate_vnode(struct sd_vnode *vnode) {
  free((void*)vnode->node);
  free(vnode);
}

typedef void (*mapper_t)(int replication_count, int hosts_count, int object_map[][NUMBER_OF_OBJECTS]);

#if 1

int vdisk_cmp(const void * elem1, const void * elem2) {
  uint64_t f = (*(struct sd_vnode **)elem1)->hash;
  uint64_t s = (*(struct sd_vnode **)elem2)->hash;
  if (f > s) return  1;
  if (f < s) return -1;
  return 0;
}

void hash_distribution(struct sd_vnode *vdisks[], int size) {
  qsort(vdisks, size, sizeof(struct sd_vnode *), vdisk_cmp);
  double occupation[size];
  uint64_t previous = 0;
  for (int i = 0; i < size; i++) {
    occupation[vdisks[i]->node->zone] = (double)(vdisks[i]->hash - previous) / UINT64_MAX;
    previous = vdisks[i]->hash;
  }

  printf("hash  ");
  for (int i = 0; i < size; i++)
    printf("  %.02f ", occupation[i]);
  printf("\n");
}

void map_with_sheepdog(int replication_count, int hosts_count, int object_map[][NUMBER_OF_OBJECTS]) {
  struct sd_vnode *vdisks[hosts_count];
  struct rb_root root;
  INIT_RB_ROOT(&root);
  for(int host = 0; host < hosts_count; host++) {
    // create a host, with a uniq addr & zone
    struct sd_node *node = (struct sd_node*)malloc(sizeof(struct sd_node));
    for (uint8_t i = 0; i < 16; i++)
      node->nid.addr[i] = i;
    node->nid.addr[15] = host;
    node->nid.port = host;
    node->zone = host;
    uint64_t hval = sd_hash(&node->nid, offsetof(typeof(node->nid), io_addr));
    // attach a vdisk to the host
    // mimic the code of node_to_vnodes(v->node, &root);
    struct sd_vnode *vdisk = (struct sd_vnode*)malloc(sizeof(struct sd_vnode));
    hval = sd_hash_next(hval);
    vdisk->hash = hval;
    vdisk->node = node;
    if (unlikely(rb_insert(&root, vdisk, rb, vnode_cmp)))
      panic("hash collison");
    vdisks[host] = vdisk;
  }

  for(int oid = 0; oid < NUMBER_OF_OBJECTS; oid++) {
    const struct sd_vnode *result[replication_count];
    memset(result, '\0', sizeof(result));
    oid_to_vnodes(oid, &root, replication_count, result);
    for(int i = 0; i < replication_count; i++) {
      assert(result[i]);
      object_map[i][oid] = result[i]->node->zone;
    }
  }
  
  hash_distribution(vdisks, hosts_count);
  for(int host = 0; host < hosts_count; host++)
    deallocate_vnode(vdisks[host]);
}
#endif

#if 0
void map_with_sheepdog(int replication_count, int hosts_count, int object_map[][NUMBER_OF_OBJECTS]) {
  struct sd_vnode *hosts[hosts_count];
  struct rb_root root;
  INIT_RB_ROOT(&root);
  for(int host = 0; host < hosts_count; host++) {
    struct sd_vnode *v = allocate_vnode();
    v->hash = sd_hash_64(host);
    v->node->zone = host;
    if (unlikely(rb_insert(&root, v, rb, vnode_cmp)))
      panic("yargl");
    hosts[host] = v;
  }

  for(int oid = 0; oid < NUMBER_OF_OBJECTS; oid++) {
    const struct sd_vnode *result[replication_count];
    memset(result, '\0', sizeof(result));
    oid_to_vnodes(oid, &root, replication_count, result);
    for(int i = 0; i < replication_count; i++) {
      assert(result[i]);
      object_map[i][oid] = result[i]->node->zone;
    }
  }
  
  for(int host = 0; host < hosts_count; host++)
    deallocate_vnode(hosts[host]);
}
#endif

void map_with_crush(int replication_count, int hosts_count, int object_map[][NUMBER_OF_OBJECTS]) {
  struct crush_map *m = crush_create();
  m->choose_local_tries = 0;
  m->choose_local_fallback_tries = 0;
  m->choose_total_tries = 50;
  m->chooseleaf_descend_once = 1;
  m->chooseleaf_vary_r = 1;
  m->chooseleaf_stable = 1;
  m->allowed_bucket_algs =
    (1 << CRUSH_BUCKET_UNIFORM) |
    (1 << CRUSH_BUCKET_LIST) |
    (1 << CRUSH_BUCKET_STRAW2);
  int root_type = 1;
  int host_type = 2;  
  int bucketno = 0;

  int hosts[hosts_count];
  int weights[hosts_count];
  int disk = 0;
  int weight = 0x10000;
  for(int host = 0; host < hosts_count; host++) {
    struct crush_bucket *b;

    b = crush_make_bucket(m, CRUSH_BUCKET_STRAW2, CRUSH_HASH_DEFAULT, host_type,
                          0, NULL, NULL);
    assert(b != NULL);
    assert(crush_bucket_add_item(m, b, disk, weight) == 0);
    assert(crush_add_bucket(m, 0, b, &bucketno) == 0);
    hosts[host] = bucketno;
    weights[host] = weight;
    disk++;
  }

  struct crush_bucket *root;
  int bucket_root;

  root = crush_make_bucket(m, CRUSH_BUCKET_STRAW2, CRUSH_HASH_DEFAULT, root_type,
                           hosts_count, hosts, weights);
  assert(root != NULL);
  assert(crush_add_bucket(m, 0, root, &bucket_root) == 0);
  assert(crush_reweight_bucket(m, root) == 0);
  
  struct crush_rule *r;
  int minsize = 1;
  int maxsize = 5;
  int number_of_steps = 3;
  r = crush_make_rule(number_of_steps, 0, 0, minsize, maxsize);
  assert(r != NULL);
  crush_rule_set_step(r, 0, CRUSH_RULE_TAKE, bucket_root, 0);
  crush_rule_set_step(r, 1, CRUSH_RULE_CHOOSELEAF_FIRSTN, replication_count, host_type);
  crush_rule_set_step(r, 2, CRUSH_RULE_EMIT, 0, 0);
  int ruleno = crush_add_rule(m, r, -1);
  assert(ruleno >= 0);

  crush_finalize(m);

  {
    int result[replication_count];
    __u32 weights[hosts_count];
    for(int i = 0; i < hosts_count; i++)
      weights[i] = 0x10000;
    int cwin_size = crush_work_size(m, replication_count);
    char cwin[cwin_size];
    crush_init_workspace(m, cwin);
    for(int x = 0; x < NUMBER_OF_OBJECTS; x++) {
      memset(result, '\0', sizeof(int) * replication_count);
      assert(crush_do_rule(m, ruleno, x, result, replication_count, weights, hosts_count, cwin) == replication_count);
      for(int i = 0; i < replication_count; i++) {
        object_map[i][x] = result[i];
      }
    }
  }
  crush_destroy(m);
}

int same_set(int object, int replication_count, int before[][NUMBER_OF_OBJECTS], int after[][NUMBER_OF_OBJECTS]) {
  for(int r = 0; r < replication_count; r++) {
    int found = 0;
    for(int s = 0; s < replication_count; s++)
      if(before[r][object] == after[s][object]) {
        found = 1;
        break;
      }
    if(!found)
      return 0;
  }
  return 1;
}

void map_compare(int replication_count, int hosts_count, mapper_t mapper) {
  int before[replication_count][NUMBER_OF_OBJECTS];
  memset(&before, '\0', sizeof(before));
  mapper(replication_count, hosts_count, &before[0]);
  int after[replication_count][NUMBER_OF_OBJECTS];
  memset(after, '\0', sizeof(after));
  mapper(replication_count, hosts_count+1, &after[0]);
  int movement[hosts_count + 1][hosts_count + 1];
  memset(movement, '\0', sizeof(movement));
  int count_before[hosts_count + 1];
  memset(count_before, '\0', sizeof(count_before));
  int count_after[hosts_count + 1];
  memset(count_after, '\0', sizeof(count_after));
  for(int object = 0; object < NUMBER_OF_OBJECTS; object++) {
    //    if(same_set(object, replication_count, &before[0], &after[0]))
    //      continue;
    for(int replica = 0; replica < replication_count; replica++) {
      count_before[before[replica][object]]++;
      count_after[after[replica][object]]++;
      if (before[replica][object] == after[replica][object])
        continue;
      movement[before[replica][object]][after[replica][object]]++;
    }
  }
  printf("    ");
  for(int host = 0; host < hosts_count + 1; host++)
    printf("    %02d ", host);
  printf("\n");
  for(int from = 0; from < hosts_count + 1; from++) {
    printf("%02d: ", from);
    for(int to = 0; to < hosts_count + 1; to++)
      printf("%6d ", movement[from][to]);
    printf("\n");
  }

  printf("before: ");
  for(int host = 0; host < hosts_count + 1; host++)
    printf("%6d ", count_before[host]);
  printf("\n");  
  printf("after:  ");
  for(int host = 0; host < hosts_count + 1; host++)
    printf("%6d ", count_after[host]);
  printf("\n");
}

int main(int argc, char* argv[]) {
  int replication_count = atoi(argv[1]);
  int hosts_count = atoi(argv[2]);
  map_compare(replication_count, hosts_count, map_with_crush);
  map_compare(replication_count, hosts_count, map_with_sheepdog);
}

/*
 * Local Variables:
 * compile-command: "cmake . && gcc -g -o compare -I crush crush/crush.c crush/mapper.c crush/builder.c crush/hash.c compare.c rbtree.c -lm && valgrind --tool=memcheck ./compare 2 10"
 * End:
 */
