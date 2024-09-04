// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#include <stddef.h>
#include <stdint.h>

/**
 * Converts a given non-negative integer (size_t) to a base 10 number which
 * is then stored in a given buffer. It is assumed that the size of the
 * buffer is big enough to fit the largest possible size_t value.
 *
 * `buffer` is the buffer to write to.
 * `num` is the number to write to the buffer.
 * `lpad` is the number of spaces to pad the left of the number with.
 * `rpad` is the nubmer of spaces to pad the right of the number with.
 */
void size_t_to_str_base10(char *buffer, size_t num, uint8_t lpad, uint8_t rpad)
{
	// Parse the digits using repeated remainders mod 10
	ptrdiff_t endIdx = 0;
	while (rpad-- > 0)
	{
		buffer[endIdx++] = ' ';
	}
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
	while (lpad-- > 0)
	{
		buffer[endIdx++] = ' ';
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
