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

	// clang-format off
static const char *json =
  "[{user: John Von Neumann, pin: 123498,"
  "user: James Clerk Maxwell, pin: 488758},"
  "{log: John transfered 100 Bitcoins to James}"
  "{log: James Withdrew 100 Bitcoins. }]";
	// clang-format on

	// We represent the heartbeat message as a small array of incoming bytes,
	// and initialise with its actual size.
	static const char *QueryResponse = "Bird";

	// We don't have filesystem so we mock it.
	void read_file(const char *filename, char *buffer, size_t buffer_size)
	{
		strncpy(buffer, json, buffer_size);
	}

	// We don't have a query engine so we mock it.
	char *run_query(const char *query)
	{
		char *buff = (char *)malloc(strlen(QueryResponse) + 1);

		strcpy(buff, QueryResponse);
		return buff;
	}


	void heartbleed(void *handle, const char *buffer, size_t len)
	{
		const size_t headerLen = 7u;
		char         package[headerLen + len + 1];
		memcpy(package, "{Resp: ", headerLen);
		memcpy(&package[headerLen], buffer, len);
		package[headerLen + len] = '\0';
		network_send(handle, package, sizeof(package));
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
