/**
 * Unit tests for logarithmic-linear histograms
 */
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "llh.h"



/** @{ */

static bool
_test_llh_init(void) {
	llh_t h = LLH_INIT;

	if (0 != llh_population_get(&h)) {
		printf("population of newly initialized histogram not 0");
		return false;
	}

	if (0 != llh_overflows_get(&h)) {
		printf("overflow count of newly initialized histogram not 0");
		return false;
	}

	return true;
}


static bool
_test_llh_record(void) {
	llh_t h = LLH_INIT;
	llh_record(&h, 0);
	if (1 != llh_population_get(&h, 0, 0)) {
		printf("inserted record, but value of bucket=0 slot=0 not 1");
		return false;
	}

	if (1 != llh_population_get(&h, 0)) {
		printf("inserted record, but population of bucket=0 not 1");
		return false;
	}

	if (1 != llh_population_get(&h)) {
		printf("inserted record, but population of histogram not 1");
		return false;
	}

	return true;
}


static bool
_test_llh_record_bucket(uint8_t bucket) {
	llh_t h = LLH_INIT;

	// Insert one element into each slot
	for (uint8_t i = 0; i < LLH_SLOTS_PER_BUCKET; i++) {
		llh_record(&h, (i * LLH_SLOT_RANGE(bucket)) + LLH_BUCKET_RANGE_LOW(bucket));
	}

	for (uint8_t i = 0; i < LLH_SLOTS_PER_BUCKET; i++) {
		if (1 != llh_population_get(&h, bucket, i)) {
			printf("value of bucket=%d slot=%d %"PRIu64", not 1", bucket, i, llh_population_get(&h, bucket, i));
			return false;
		}
	}

	if (LLH_SLOTS_PER_BUCKET != llh_population_get(&h, bucket)) {
		printf("value of bucket=%d not %d", bucket, LLH_SLOTS_PER_BUCKET);
		return false;
	}

	if (LLH_SLOTS_PER_BUCKET != llh_population_get(&h, bucket)) {
		printf("population of histogram not %d", LLH_SLOTS_PER_BUCKET);
		return false;
	}

	return true;
}


static bool
_test_llh_record_slot(uint8_t slot) {
	llh_t h = LLH_INIT;

	// Insert one element into each bucket in this slot
	for (uint8_t i = 0; i < LLH_BUCKETS; i++)
		llh_record(&h, (slot * LLH_SLOT_RANGE(i)) + LLH_BUCKET_RANGE_LOW(i));

	for (uint8_t i = 0; i < LLH_BUCKETS; i++) {
		if (1 != llh_population_get(&h, i, slot)) {
			printf("value of bucket=%d slot=%d %"PRIu64", not 1", i, slot, llh_population_get(&h, i, slot));
			return false;
		}
	}

	for (uint8_t i = 0; i < LLH_BUCKETS; i++) {
		if (1 != llh_population_get(&h, i)) {
			printf("value of bucket=%d not 1", i);
			return false;
		}
	}

	if (LLH_BUCKETS != llh_population_get(&h)) {
		printf("population of histogram not %d", LLH_BUCKETS);
		return false;
	}

	return true;
}


static bool
_test_llh_overflow(void) {
	llh_t h = LLH_INIT;

	llh_record(&h, LLH_BUCKET_RANGE_HIGH(LLH_BUCKETS) + 1);

	if (1 != llh_overflows_get(&h)) {
		printf("inserted overflow value, but overflow counter not 1");
		return false;
	}

	return true;
}


static void
_test_llh_perf(void) {
	llh_t h = LLH_INIT;

	struct timespec st, ft;
	clock_gettime(CLOCK_MONOTONIC_RAW, &st);

	for (uint64_t v = 0; v < 100000000; v++)
		llh_record(&h, v);

	clock_gettime(CLOCK_MONOTONIC_RAW, &ft);

	uint64_t st_ns = (uint64_t)(st.tv_sec * 1000000000) + (uint64_t)st.tv_nsec;
	uint64_t ft_ns = (uint64_t)(ft.tv_sec * 1000000000) + (uint64_t)ft.tv_nsec;

	printf("PERF: inserted 100,000,000 records in %0.4f sec\n", (double)(ft_ns - st_ns) / 1000000000);
}


int
main(int argc __attribute__((unused)), char** argv __attribute__((unused))) {
	bool is_success = true;

	// Test for correctness of a freshly initialized histogram
	printf("TEST: histogram initialization: ");
	if (!_test_llh_init()) {
		is_success = false;
		printf(" FAIL\n");
	} else
		printf("OK\n");

	// Test for correctness of a single insertion
	printf("TEST: single insertion: ");
	if (!_test_llh_record()) {
		is_success = false;
		printf(" FAIL\n");
	} else
		printf("OK\n");

	// Test for correctness of inserting to every slot in each bucket
	printf("TEST: inserting into slots 0-15 in bucket: ");
	for (uint8_t i = 0; i < LLH_BUCKETS; i++) {
		printf("%d ", i);
		if (!_test_llh_record_bucket(i)) {
			is_success = false;
			printf(" FAIL ");
		} else
			printf("OK ");
	}
	printf("\n");

	// Test for correctness of inserting to each slot in every bucket
	printf("TEST: inserting into every bucket in slot: ");
	for (uint8_t i = 0; i < LLH_SLOTS_PER_BUCKET; i++) {
		printf("%d ", i);
		if (!_test_llh_record_slot(i)) {
			is_success = false;
			printf(" FAIL ");
		} else
			printf("OK ");
	}
	printf("\n");

	// Test for correctness of overflows
	printf("TEST: overflow counting: ");
	if (!_test_llh_overflow()) {
		is_success = false;
		printf(" FAIL\n");
	} else
		printf("OK\n");

	// Performance "test"
	_test_llh_perf();

	printf("Unit tests: %s\n", (is_success) ? "OK" : " FAIL");
	return (is_success) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/** @} */
