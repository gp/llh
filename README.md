LLH
===

What's a logarithmic-linear histogram?
--------------------------------------

Histograms have been widely used to estimate the probability distribution of a continuous variable. Choosing the bin size is difficult: using a logarithmic progression gives a wide range, but at a cost to resolution (particularly in the upper part of the range). Histograms using a linear progression require a large amount of space to compensate for their decreased range.

The logarithmic-linear histogram is a compromise between the two, and provides improved resolution while maintaining a wide range and a reduced space requirement. It does this by using a two-level binning scheme. The top-level bins (which are called _buckets_) are sized logarithmically (base 2). Each bucket also has a set of second-level bins (which are called _slots_) -- 16 per bucket -- which are sized linearly within the bin.


Usage
-----

The entire implementation is contained in the header file `llh.h`; there's no library to link against.

Create a new LLH by initializing it with the provided static initializer, or by just allocating some cleared memory for it:

	llh_t histogram1 = LLH_INIT;
	llh_t* histogram2 = calloc(1, sizeof(llh_t));

To record an event in a histogram, call `llh_record`:

	llh_record((llh_t*)histogram, (uint64_t)value);

To get data back out of a histogram, you have three choices, depending on the degree of aggregation desired -- total over the entire histogram, total for one bucket, or the value in a specific slot.

	(uint64_t)llh_population_get((llh_t*)histogram);
	(uint64_t)llh_population_get((llh_t*)histogram, bucket);
	(uint64_t)llh_population_get((llh_t*)histogram, bucket, slot);

Finally, you can use the provided accessor to get the number of overflows:

	(uint32_t)llh_overflows_get((llh_t*)histogram);


Tests
-----

To run the provided unit tests, use the provided `Makefile`:

	make clean && make && bin/llh_test

Implementation notes
--------------------

Incrementing histogram values is done by means of an atomic increment. If your platform does not support the `INCL` instruction, you'll need to compile with the `LLH_UNSAFE_INCREMENT` flag set.
