// Copyright lowRISC Contributors.
// SPDX-License-Identifier: Apache-2.0

#define MALLOC_QUOTA 1024 // Quota of 1 KiB

#include <compartment.h>

DECLARE_AND_DEFINE_ALLOCATOR_CAPABILITY(__second_malloc_capability,
                                        MALLOC_QUOTA * 2)
DECLARE_AND_DEFINE_ALLOCATOR_CAPABILITY(__third_malloc_capability,
                                        MALLOC_QUOTA * 4)
DECLARE_AND_DEFINE_ALLOCATOR_CAPABILITY(__fourth_malloc_capability,
                                        MALLOC_QUOTA * 8)
DECLARE_AND_DEFINE_ALLOCATOR_CAPABILITY(__fifth_malloc_capability,
                                        MALLOC_QUOTA * 16)
#define MALLOC_CAPABILITY_2 STATIC_SEALED_VALUE(__second_malloc_capability)
#define MALLOC_CAPABILITY_3 STATIC_SEALED_VALUE(__third_malloc_capability)
#define MALLOC_CAPABILITY_4 STATIC_SEALED_VALUE(__fourth_malloc_capability)
#define MALLOC_CAPABILITY_5 STATIC_SEALED_VALUE(__fifth_malloc_capability)

#define MALLOC_WITH_CAPABILITY_FUNC(name, malloc_num)                          \
	static inline void *malloc##malloc_num(size_t size)                        \
	{                                                                          \
		Timeout t = {0, MALLOC_WAIT_TICKS};                                    \
		void   *ptr =                                                          \
		  heap_allocate(&t, name, size, AllocateWaitRevocationNeeded);         \
		if (!__builtin_cheri_tag_get(ptr))                                     \
		{                                                                      \
			ptr = NULL;                                                        \
		}                                                                      \
		return ptr;                                                            \
	}

MALLOC_WITH_CAPABILITY_FUNC(MALLOC_CAPABILITY_2, 2)
MALLOC_WITH_CAPABILITY_FUNC(MALLOC_CAPABILITY_3, 3)
MALLOC_WITH_CAPABILITY_FUNC(MALLOC_CAPABILITY_4, 4)
MALLOC_WITH_CAPABILITY_FUNC(MALLOC_CAPABILITY_5, 5)

/// Thread entry point.
[[noreturn]] void __cheri_compartment("malloc_many") entry_point()
{
	void *mem = malloc(MALLOC_QUOTA * sizeof(char));
	free(mem);
	void *mem2 = malloc2(MALLOC_QUOTA * 2 * sizeof(char));
	free(mem2);
	void *mem3 = malloc3(MALLOC_QUOTA * 4 * sizeof(char));
	free(mem3);
	void *mem4 = malloc4(MALLOC_QUOTA * 8 * sizeof(char));
	free(mem4);
	void *mem5 = malloc5(MALLOC_QUOTA * 16 * sizeof(char));
	free(mem5);

	int x = 0;
	while (true)
	{
		x = x + 1;
	};
}