// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <cheri.hh>
#include <compartment.h>
#include <debug.hh>
#include <functional>
#include <futex.h>
#include <interrupt.h>
#include <platform-uart.hh>
#include <thread.h>

using Debug   = ConditionalDebug<true, "Uart Test">;
using UartPtr = volatile OpenTitanUart *;

bool loopback_test(UartPtr uart)
{
	uart->init();
	uart->fifos_clear();
	uart->parity();
	uart->loopback();

	const char TestString[] = "test string";
	for (char c : TestString)
	{
		uart->blocking_write(c);
	}
	for (char c : TestString)
	{
		if (uart->blocking_read() != c)
		{
			return false;
		}
	}
	return true;
}

bool interrupt_state_test(UartPtr uart)
{
	uart->init();
	uart->fifos_clear();
	uart->parity();
	uart->transmit_watermark(OpenTitanUart::TransmitWatermark::Level4);

	uint32_t count = 0;
	while (uart->interruptState & OpenTitanUart::InterruptTransmitWatermark)
	{
		uart->blocking_write('x');
		++count;
	};

	return count == 5;
}

bool __cheri_libcall uart_tests()
{
	UartPtr uart1 = MMIO_CAPABILITY(OpenTitanUart, uart1);

	std::pair<const char *, std::function<bool(UartPtr)>> testFunctions[] = {
	  {"loopback test", loopback_test},
	  {"interrupt state test", interrupt_state_test},
	};
	for (auto [name, function] : testFunctions)
	{
		Debug::log("Running {}", name);
		if (!function(uart1))
		{
			return false;
		}
	};
	Debug::log("All tests passed");
	return true;
}
