// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include "uart_tests.hh"
#include <debug.hh>
#include <platform-uart.hh>
#include <thread.h>

using Debug = ConditionalDebug<true, "Sonata Test Runner">;

[[noreturn]] void finish_running(const char *message)
{
	Debug::log(message);

	while (true)
	{
		Timeout t{100};
		thread_sleep(&t);
	}
}

void check_result(bool result)
{
	if (result)
	{
		return;
	}
	finish_running("One or more tests failed");
}

[[noreturn]] void __cheri_compartment("test_runner") run_tests()
{
	check_result(uart_tests());
	finish_running("All tests finished");
}

extern "C" ErrorRecoveryBehaviour
compartment_error_handler(ErrorState *frame, size_t mcause, size_t mtval)
{
	auto [exceptionCode, registerNumber] = CHERI::extract_cheri_mtval(mtval);
	Debug::log(
	  "Exception[ mcause({}), {}, {} ]", mcause, exceptionCode, registerNumber);
	finish_running("One or more tests failed");
	return ErrorRecoveryBehaviour::ForceUnwind;
}
