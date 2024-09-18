// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#define MALLOC_QUOTA 4096 // Quota of 4 KiB

#include <compartment.h>

/// Thread entry point.
[[noreturn]] void __cheri_compartment("malloc4096") entry_point()
{
	void *mem = malloc(4096 * sizeof(char));
	free(mem);

	int x = 0;
	while (true)
	{
		x = x + 1;
	};
}
