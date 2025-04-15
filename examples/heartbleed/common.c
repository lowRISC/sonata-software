// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "common.h"

#include <stdlib.h>

#if __has_builtin(__builtin_launder)
#	define LAUNDER(_p_) __builtin_launder(_p_)
#else
#	define LAUNDER(_p_) (_p_)
#endif

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

	// We launder here as a fence to attempt to stop the compiler optimising
	// around undefined behaviour for demo consistency.
	char *heartbleed(const char *msg, size_t len)
	{
		if (len == 0u)
		{
			return NULL;
		}
		char *buff = (char *)malloc(*LAUNDER(&len));

		// We copy the non-terminated string to the new buffer. This should
		// cause an error on a CHERIoT platform when given a `len` that is
		// too high, but will leak arbitrary memory on legacy platforms.
		memcpy(buff, msg, *LAUNDER(&len));
		return buff;
	}

	void size_t_to_str_base10(char *buffer, size_t num)
	{
		// Parse the digits using repeated remainders mod 10
		ptrdiff_t endIdx = 0;
		if (num == 0)
		{
			buffer[endIdx++] = '0';
		}
		while (num != 0)
		{
			int remainder    = num % 10;
			buffer[endIdx++] = '0' + remainder;
			num /= 10;
		}
		buffer[endIdx--] = '\0';

		// Reverse the generated string
		ptrdiff_t startIdx = 0;
		while (startIdx < endIdx)
		{
			char swap          = buffer[startIdx];
			buffer[startIdx++] = buffer[endIdx];
			buffer[endIdx--]   = swap;
		}
	}

#ifdef __cplusplus
}
#endif //__cplusplus
