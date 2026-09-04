// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0
//
// CWE-190: Integer Overflow leading to Out-of-Bounds Write
//
// This compartment uses a large (wrapping) integer index to access a
// stack-allocated array well beyond its bounds.  On a 32-bit CHERIoT system,
// the address arithmetic wraps around but the resulting address is nowhere
// near the 32-byte bounds of buf[], so CHERIoT raises a bounds exception.
//
// CHERIoT protection: buf[] has precise bounds of 32 bytes (8 × 4).
// ptr[0xFFFFFFFF] computes address = ptr + 0xFFFFFFFF * 4 = ptr + 0xFFFFFFFC
// (wrapping on 32-bit).  That address is outside the 32-byte bounds → CHERI
// bounds exception.
//
// Note: volatile on `idx` prevents constant folding that would allow the
// compiler to prove the access is UB and eliminate it.

#include "juliet_cwe.h"
#include <compartment.h>
#include <debug.hh>
#include <new>
#include <stdint.h>

using Debug = ConditionalDebug<true, "CWE-190 Int Overflow OOB">;

extern "C" ErrorRecoveryBehaviour
compartment_error_handler(ErrorState *frame, size_t mcause, size_t mtval)
{
	Debug::log(
	  "CHERI exception caught: mcause={} mtval={} pcc={}", mcause, mtval, frame->pcc);
	return ErrorRecoveryBehaviour::ForceUnwind;
}

void *test_cwe190_int_overflow(int)
{
	int buf[8];
	// std::launder gives the compiler no information about buf's bounds,
	// preventing it from optimising away the OOB access.
	int             *ptr = std::launder(buf);
	volatile uint32_t idx = 0xFFFFFFFFU;
	__c11_atomic_signal_fence(__ATOMIC_SEQ_CST);
	// On 32-bit RISC-V: ptr + idx * sizeof(int) = ptr + 0xFFFFFFFC
	// The resulting address is outside buf's 32-byte CHERI bounds.
	ptr[idx] = 0; // CHERI bounds exception: index 0xFFFFFFFF is OOB for buf[8]
	__c11_atomic_signal_fence(__ATOMIC_SEQ_CST);
	return nullptr; // unreachable
}
