/*
 * cuckoo.h
 *
 *  Created on: Sep 2, 2014
 *      Author: yonch
 */

#ifndef CUCKOO_H_
#define CUCKOO_H_

#include <stdint.h>
#include <errno.h>
#include <stddef.h>

struct cuckoo_bucket {
	uint8_t tags[16];
	uint8_t ind_upper[16];
	uint16_t ind_lower[16];
};

struct cuckoo_elem {
	void *elem;
	uint64_t key;
};

struct cuckoo_htable {
	/* the hash table has (1 << n_bits) buckets */
	uint32_t n_bits;

	/* a mask to get the lower n_bits */
	uint32_t bucket_mask;

	/* the buckets */
	struct cuckoo_bucket *buckets;

	/* elements */
	struct cuckoo_elem *elems;

	/* elem free list head */
	struct cuckoo_elem *free_list;

	void *buckets_mem; /* non-aligned memory */
};

/**
 * Initializes a cuckoo hash
 * @param ht: the hash table to initialize
 * @param n_bits: the hash will have (1 << n_bits) buckets
 * @return: 0 on success,
 * 			-ENOMEM when cannot allocate memory,
 * 			-EINVAL when n_bits too big
 */
static inline int cuckoo_init(struct cuckoo_htable *ht, uint32_t n_bits);

/**
 * Frees up memory of the hash table. Does not touch the void*'s stored as
 *   elements within the hash table
 */
static inline void cuckoo_destroy(struct cuckoo_htable *ht);

/**
 * Inserts an element to the hash table
 * @param ht: the hash table
 * @param elem: the element to insert
 * @param key: the hash of the element
 * @return: 0 on success, -ENOMEM if hash is too full
 */
static inline int cuckoo_insert(struct cuckoo_htable *ht, void *elem, uint64_t key);

/**
 * Deletes a key from the hash table, and returns its value.
 * @param ht: the hash table
 * @param key: the key to be removed
 * @returns: the key associated with the value.
 * @note: assumes that key exists within the table, otherwise return is undefined
 */
static inline void *cuckoo_delete(struct cuckoo_htable *ht, uint64_t key);

/**
 * Retrieves the value associated with the given key.
 */
static inline void *cuckoo_get(struct cuckoo_htable *ht, uint64_t key);


/*****************
 * IMPLEMENTATION
 */

/* FNV hash */
#define FNV_32_PRIME (0x01000193)
#define FNV1_32_INIT (0x811c9dc5)


static inline int cuckoo_init(struct cuckoo_htable *ht, uint32_t n_bits)
{
	uint64_t num_buckets = (1 << n_bits);
	uint64_t i;

	assert(ht != NULL);

	if (n_bits > 24)
		return -EINVAL;

	ht->n_bits = n_bits;
	ht->bucket_mask = ((1 << n_bits) - 1);

	ht->buckets_mem = malloc(sizeof(struct cuckoo_bucket) * num_buckets + 63);
	if (ht->buckets_mem == NULL)
		goto cannot_allocate_buckets;
	/* make sure buckets is aligned to cache_line boundary (64 bytes) */
	ht->buckets = (((char *)ht->buckets_mem) + 63) & ~(63uLL);

	ht->elems = malloc(sizeof(struct cuckoo_elem) * num_buckets);
	if (ht->elems == NULL)
		goto cannot_allocate_elems;

	/* set up free list */
	for (i = 0; i < num_buckets - 1; i++)
		ht->elems[i].elem = &ht->elems[i+1];
	ht->elems[num_buckets - 1].elem = NULL;
	ht->free_list = &ht->elems[0];

	/* initialize buckets */
	for (i = 0; i < num_buckets; i++)
		memset(&ht->buckets[i].tags[0], 0xFF, 16);

	return 0;

cannot_allocate_elems:
	free(ht->buckets_mem);
cannot_allocate_buckets:
	return -ENOMEM;
}

static inline void cuckoo_destroy(struct cuckoo_htable *ht)
{
	free(ht->buckets_mem);
	free(ht->elems);
	ht->buckets_mem = ht->buckets = ht->elems = ht->free_list = NULL;
}

static inline uint32_t _other_bucket(struct cuckoo_htable *ht, uint32_t bucket,
		uint8_t tag)
{
	return (((FNV1_32_INIT ^ tag) * FNV_32_PRIME) ^ bucket) & ht->bucket_mask;
}

static inline int cuckoo_insert(struct cuckoo_htable *ht, void *elem, uint64_t key)
{
	uint32_t bucket1 = key & ht->bucket_mask;
	uint8_t tag = key >> 24;
	uint32_t bucket2 = _other_bucket(ht, key, tag);


}

#endif /* CUCKOO_H_ */
