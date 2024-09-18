// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#define MALLOC_QUOTA 2048 // Quota of 2 KiB

#include <compartment.h>

/// Thread entry point.
[[noreturn]] void __cheri_compartment("malloc2048") entry_point()
{
	void *mem = malloc(2048 * sizeof(char));
	free(mem);

	int x = 0;
	while (true)
	{
		x = x + 1;
	};
}
