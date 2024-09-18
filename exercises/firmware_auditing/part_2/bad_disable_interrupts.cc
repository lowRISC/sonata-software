// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <compartment.h>

[[cheri::interrupt_state(disabled)]] int not_allowed()
{
	int x = 0;
	x += 1;
	return x;
}

[[gnu::noinline]] int other_function()
{
	return 1000000;
}

/// Thread entry point.
void __cheri_compartment("bad_disable_interrupts") entry_point()
{
	int y = not_allowed();
	other_function();
	while (y < other_function())
	{
		y += 1;
	}
}
