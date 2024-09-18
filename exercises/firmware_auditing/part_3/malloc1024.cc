// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#define MALLOC_QUOTA 1024 // Quota of 1 KiB

#include <compartment.h>

/// Thread entry point.
[[noreturn]] void __cheri_compartment("malloc1024") entry_point()
{
	void *mem = malloc(1024 * sizeof(char));
	free(mem);

	int x = 0;
	while (true)
	{
		x = x + 1;
	};
}
