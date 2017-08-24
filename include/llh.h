/**
 * Reference implementation of logarithmic-linear histograms
 * @author Gian-Paolo Musumeci <gp@cmpxchg16b.io>
 */
#pragma once
#include <stdint.h>



/** @{ */

// Version information
#define LLH_VERSION_MAJOR 1
#define LLH_VERSION_MINOR 0
#define LLH_VERSION_MICRO 0

// The version represented as a string
#define LLH_VERSION_STRING "1.0.0"

// The version represented as a 3-byte hexadecimal number, e.g. 0x010203 =
// 1.2.3. This can be used for easy version comparisons.
#define LLH_VERSION_HEX ((LLH_VERSION_MAJOR << 16) | (LLH_VERSION_MINOR << 8) | (LLH_VERSION_MICRO))


// Definitions
/** Opaque type definition for a logarithmic-linear histogram */
typedef struct _llh llh_t;

/** Static initializer for a histogram */
#define LLH_INIT { .overflows = 0 }

/** The number of buckets (logarithmic) in a histogram */
#define LLH_BUCKETS 24

/** The number of slots per bucket (linear) in a histogram */
#define LLH_SLOTS_PER_BUCKET 16

/** The lowest possible value that can be stored in a specific bucket */
#define LLH_BUCKET_RANGE_LOW(_b) ((0 == (_b) ? 0 : (1 << ((_b) + LLH_SCALE))))

/** The highest possible value that can be stored in a specific bucket */
#define LLH_BUCKET_RANGE_HIGH(_b) ((1 << ((_b) + LLH_SCALE + 1)) - 1)

/** The range of a slot in a particular bucket */
#define LLH_SLOT_RANGE(_b) ((LLH_BUCKET_RANGE_HIGH(_b) - LLH_BUCKET_RANGE_LOW(_b) + 1) / LLH_SLOTS_PER_BUCKET)


// Prototypes
/**
 * Record an event in a histogram
 * @param histogram A non-NULL pointer to a llh_t
 * @param value The value to record
 */
static inline void llh_record(llh_t* histogram, uint64_t value) __attribute__((nonnull(1)));

/**
 * Get the population of a histogram
 * @param histogram A non-NULL pointer to a llh_t
 * @returns The total population of the histogram
 */
static inline uint64_t llh_population_get(llh_t* histogram) __attribute__((nonnull(1))) __attribute__((overloadable));

/**
 * Get the population of a bucket in a histogram
 * @param histogram A non-NULL pointer to a llh_t
 * @param bucket The index of a bucket
 * @returns The total number of events in all slots of the specified bucket of
 *   the histogram
 */
static inline uint64_t llh_population_get(llh_t* histogram, uint8_t bucket) __attribute__((nonnull(1))) __attribute__((overloadable));

/**
 * Get the population of a slot in a bucket in a histogram
 * @param histogram A non-NULL pointer to a llh_t
 * @param bucket The index of a bucket
 * @param slot The index of a slot
 * @returns The number of events in the specified slot of the specified bucket
 *   of the histogram
 */
static inline uint64_t llh_population_get(llh_t* histogram, uint8_t bucket, uint8_t slot) __attribute__((nonnull(1))) __attribute__((overloadable));

/**
 * Get the number of overflows in a histogram
 * @param histogram A non-NULL pointer to a llh_t
 * @returns The number of events resulting in an overflow
 */
static inline uint32_t llh_overflows_get(llh_t* histogram) __attribute__((overloadable));


// Implementation
#define LLH_SCALE (__builtin_ctz(LLH_SLOTS_PER_BUCKET) - 1)

struct _llh {
	uint32_t v[LLH_BUCKETS][LLH_SLOTS_PER_BUCKET];
	uint32_t overflows;
};

static inline void
_incrl(volatile int32_t* a) {
	__asm__ __volatile__ ("lock; incl %[a]" : [a]"+m"(*a));
}

static inline void
llh_record(llh_t* h, uint64_t v) {
	uint64_t bucket, slot;

	// Find the bucket for this value. Logically, we left-shift the value by the
	// scale factor, since each bucket must contain at least
	// (LLH_SLOTS_PER_BUCKET - 1) elements; then we take the binary logarithm of
	// the value. There is no builtin for the x86 BSR instruction (Bit Scan
	// Reverse, aka binary logarithm), so here it is implemented with CLZ (Count
	// Leading Zeros). Subtracting the number of leading zeros from the word
	// size gives us the position of the first bit set. (Note that for numbers
	// of the form 2x - 1, 63 - x == x ^ 63)
	// NB CLZLL doesn't like being passed 0. So, we check for that case. :)
	bucket = (v < LLH_SLOTS_PER_BUCKET) ? 0 : (uint64_t)__builtin_clzll(v >> LLH_SCALE) ^ 63ULL;
	if (LLH_BUCKETS > bucket) {
		// Now we need to find the slot index. If the bucket is zero, then we
		// can just use the value directly. Otherwise, we need to do some
		// bitwise trickery.
		// This is done by masking off the most significant bit to find the
		// "remainder" of the logarithm. We have to take the scale factor into
		// account, so this mask is given by 2^(bucket + SCALE_FACTOR) - 1; a
		// simple AND with the value applies the mask and gives the numerator.
		// Now that we know what the remainder of the logarithm is, we need to
		// know how big each slot is for this bucket. The easiest way to get it
		// is to take the total range of the bucket, which is 2^(bucket +
		// SCALE_FACTOR), and divide it by the number of slots in each bucket.
		// This gives the denominator.
		// Computing the ratio gives the slot index.
		if (0 == bucket)
			slot = v;
		else
			slot = (v & ((1 << (bucket + LLH_SCALE)) - 1)) / ((1 << (bucket + LLH_SCALE)) / LLH_SLOTS_PER_BUCKET);
#ifdef LLH_UNSAFE_INCREMENT
		h->v[bucket][slot]++;
#else
		_incrl((int32_t*)&h->v[bucket][slot]);
#endif
	} else {
		// Increment the overflow counter
#ifdef LLH_UNSAFE_INCREMENT
		h->overflows++;
#else
		_incrl((int32_t*)&h->overflows);
#endif
	}

	return;
}


static inline uint64_t __attribute__((overloadable))
llh_population_get(llh_t* h) {
	uint64_t p = 0;
	for (uint8_t i = 0; i < LLH_BUCKETS; i++) {
		for (uint8_t j = 0; j < LLH_SLOTS_PER_BUCKET; j++)
			p += h->v[i][j];
	}
	return p;
}


static inline uint64_t __attribute__((overloadable))
llh_population_get(llh_t* h, uint8_t bucket) {
	uint64_t p = 0;
	for (uint8_t i = 0; i < LLH_SLOTS_PER_BUCKET; i++)
		p += h->v[bucket][i];
	return p;
}


static inline uint64_t __attribute__((overloadable))
llh_population_get(llh_t* h, uint8_t bucket, uint8_t slot) {
	return (uint64_t)h->v[bucket][slot];
}


static inline uint32_t __attribute__((overloadable))
llh_overflows_get(llh_t* h) {
	return h->overflows;
}

/** @} */
