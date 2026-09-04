// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0
//
// CWE-121: Stack Buffer Overflow
//
// This compartment deliberately writes past the end of a 16-element
// stack-allocated array.  CHERIoT hardware detects the bounds violation and
// raises a CHERI exception.  The compartment_error_handler catches it and
// returns ForceUnwind, so the caller receives an invalid return capability.
//
// CHERIoT protection: stack capabilities have precise bounds set at function
// entry.  ptr[32] (byte offset 128) is outside the 64-byte bounds of buf →
// CHERI bounds exception (mcause = 0x1c, CHERI MTVAL = bounds violation).

#include "juliet_cwe.h"
#include <compartment.h>
#include <debug.hh>
#include <new>

using Debug = ConditionalDebug<true, "CWE-121 Stack BOF">;

extern "C" ErrorRecoveryBehaviour
compartment_error_handler(ErrorState *frame, size_t mcause, size_t mtval)
{
	Debug::log(
	  "CHERI exception caught: mcause={} mtval={} pcc={}", mcause, mtval, frame->pcc);
	return ErrorRecoveryBehaviour::ForceUnwind;
}

void *test_cwe121_bof(int)
{
	int buf[16];
	// std::launder prevents the compiler from treating the out-of-bounds
	// access as undefined behaviour it can eliminate.  The atomic fences
	// prevent the store from being hoisted or sunk out of the load barrier
	// check window.
	int *ptr = std::launder(buf);
	__c11_atomic_signal_fence(__ATOMIC_SEQ_CST);
	ptr[32] = 0; // bounds violation: buf has 16 ints (64 bytes); index 32 is OOB
	__c11_atomic_signal_fence(__ATOMIC_SEQ_CST);
	return nullptr; // unreachable: CHERI exception fires at ptr[32]
}
