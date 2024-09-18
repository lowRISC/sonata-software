// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <compartment.h>

[[gnu::noinline]] [[cheri::interrupt_state(disabled)]] int
run_without_interrupts(int x)
{
	return x + 1;
}

[[gnu::noinline]] [[cheri::interrupt_state(disabled)]] void
also_without_interrupts(int *x)
{
	*x = 1;
}

[[gnu::noinline]] void other_function_one()
{
	int x = 3;
}

[[gnu::noinline]] int other_function_two()
{
	return 3;
}

[[gnu::noinline]] int other_function_three(int arg1, int arg2)
{
	return arg1 + arg2;
}

/// Thread entry point.
void __cheri_compartment("disable_interrupts") entry_point()
{
	other_function_one();
	int y = other_function_two();
	y     = run_without_interrupts(3);
	y += other_function_three(4, 7);
	also_without_interrupts(&y);
}