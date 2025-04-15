// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#ifndef _HEARTBLEED_COMMON_H_
#define _HEARTBLEED_COMMON_H_

#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

	/**
	 * @brief Contains the buggy "heartbleed"-like implementation. Allocates
	 * some space on the heap to store the copied string, entirely trusting the
	 * user-provided length parameter without any sanitisation. This means that
	 * if a length longer than the message is given, the remainder of the buffer
	 * will be uninitialised and contain values from memory.
	 *
	 * @param msg The message to echo back.
	 * @param len The unsanitized length of the message.
	 * @return A heap-allocated copy of the message. Simulates a response
	 * packet.
	 */
	char *heartbleed(const char *msg, size_t len);

	/**
	 * @brief Converts a given size_t to an equivalent string representing its
	 * unsigned base 10 representation in the given buffer.
	 *
	 * @param buffer The buffer/string to write the converted number to. Will
	 * terminate at the end of the string.
	 * @param num The size_t number to convert
	 */
	void size_t_to_str_base10(char *buffer, size_t num);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _HEARTBLEED_COMMON_H_
