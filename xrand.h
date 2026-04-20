#ifndef XRAND_H
#define XRAND_H

#include <assert.h>
#include <stdlib.h>
#include <time.h>

size_t xs;

void init_xorshift(void) {
	xs = 0xDEADBEEF ^ time(NULL);
}

void init_xorshift_seed(size_t seed) {
	xs = seed;
}

size_t xorshift() {
	xs ^= xs << 13;
	xs ^= xs >> 17;
	xs ^= xs << 5;

	return xs;
}

int xs_rand(int l, int r) {
	assert(l < r);

	int len = r - l;

	return ((xorshift() % len + len) % len) + l;
}

#endif // XRAND_H
