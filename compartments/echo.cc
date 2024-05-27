// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <compartment.h>
//#include <debug.hh>
#include <platform-uart.hh>
#include <thread.h>

/// Expose debugging features unconditionally for this compartment.
// using Debug = ConditionalDebug<true, "led walk compartment">;

/// Thread entry point.
[[noreturn]] void __cheri_compartment("echo") entry_point()
{
	auto uart = MMIO_CAPABILITY(OpenTitanUart<>, uart);

	char ch = '\n';
	while (true)
	{
		ch = uart->blocking_read();
		uart->blocking_write(ch);
	}
}
