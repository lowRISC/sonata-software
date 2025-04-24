// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#ifndef _HEARTBLEED_COMMON_H_
#define _HEARTBLEED_COMMON_H_

#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

	extern void network_send(void *handle, const char *package, size_t len);

	/**
	 * @brief Run a query and allocate a buffer with the response, the buffer
	 * should be deallocated by the caller.
	 *
	 * @param query The query to be executed.
	 * @return A heap-allocated copy of the message. Simulates a response
	 * packet.
	 */
	char *run_query(const char *query);

	/**
	 * @brief Format the response package, the buffer is copied to the response
	 * package based on the length, if larger than the buffer, sensitive
	 * information can be leaked throught the network.
	 *
	 * This function requires that the caller implements the function
	 * `network_send`.
	 *
	 * @param handle A pointer to a handle that will be forward to the
	 * network_send.
	 * @param buffer The buffer to be sent.
	 * @param buffer_size The size of the buffer.
	 * @return A heap-allocated copy of the message. Simulates a response
	 * packet.
	 */
	void heartbleed(void *handle, const char *buffer, size_t len);

	/**
	 * @brief Converts a given size_t to an equivalent string representing its
	 * unsigned base 10 representation in the given buffer.
	 *
	 * @param buffer The buffer/string to write the converted number to. Will
	 * terminate at the end of the string.
	 * @param num The size_t number to convert
	 */
	void size_t_to_str_base10(char *buffer, size_t num);

	/**
	 * @brief Reads a file content in the given buffer.
	 *
	 * @param filename The name of the file.
	 * @param buffer The buffer/string to write the file content.
	 * @param buffer_size The of the buffer.
	 */
	void read_file(const char *filename, char *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _HEARTBLEED_COMMON_H_
