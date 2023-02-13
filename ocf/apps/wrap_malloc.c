#include <malloc.h>
#include <stddef.h>
#include <stdio.h>
#include "mbedtls/platform.h"

#include "ca821x_log.h"
#include "oc_blockwise.h"

extern void *__real_malloc(size_t);
extern void  __real_free(void *);
extern void *__real_calloc(size_t, size_t);
extern void *__real_mbedtls_calloc(size_t nmemb, size_t size);

struct metadata
{
	void  *buffer_addr;
	size_t size;
	void  *return_addr;
};

#define MAX_ALLOC_COUNT (1)
struct metadata allocs[MAX_ALLOC_COUNT];

void print_top_allocs()
{
	printf("Top %d bufs:\n", MAX_ALLOC_COUNT);

	for (int i = 0; i < MAX_ALLOC_COUNT; ++i)
	{
		if (allocs[i].buffer_addr)
			printf("b %d -> %p @ *%p\n", allocs[i].size, allocs[i].buffer_addr, allocs[i].return_addr);
	}
}

void *__wrap_malloc(size_t size)
{
	void *buffer = __real_malloc(size);

	struct mallinfo info;
	info             = mallinfo();
	size_t remaining = Heap_Size - info.uordblks;

	if (buffer == 0)
	{
		printf("m %d -> %p @ *%p\n", size, buffer, __builtin_return_address(0));
		printf("h 0x%x (%d)\n", remaining, remaining);
		print_top_allocs();
	}
	else
	{
		struct metadata *worst = NULL;
		for (int i = 0; i < MAX_ALLOC_COUNT; ++i)
		{
			if (allocs[i].buffer_addr == NULL || allocs[i].size < worst->size)
				worst = allocs + i;
		}

		if (worst)
		{
			worst->buffer_addr = buffer;
			worst->size        = size;
			worst->return_addr = __builtin_return_address(0);
		}
	}

	return buffer;
}

void __wrap_free(void *buffer)
{
	__real_free(buffer);

	struct mallinfo info;
	info             = mallinfo();
	size_t remaining = Heap_Size - info.uordblks;

	// The printing of the mbedTLS messages slows down the execution of
	// the stack so much, that tests start failing due to timeouts. Therefore,
	// we have chosen to disable these.
	/*
    //if (__builtin_return_address(0) != mbedtls_free + 0x9)
    {
        printf("f %p @ *%p\n", buffer, __builtin_return_address(0));
        printf("h 0x%x (%d)\n", remaining, remaining);
    }
    */

	if (buffer != 0)
	{
		for (int i = 0; i < MAX_ALLOC_COUNT; ++i)
		{
			if (allocs[i].buffer_addr == buffer)
			{
				allocs[i].buffer_addr = NULL;
				allocs[i].return_addr = NULL;
				allocs[i].size        = 0;
			}
		}
	}
}

void *__wrap_calloc(size_t num, size_t size)
{
	void *buffer = __real_calloc(num, size);

	struct mallinfo info;
	info             = mallinfo();
	size_t remaining = Heap_Size - info.uordblks;

	//if (__builtin_return_address(0) != mbedtls_calloc + 0x9 || buffer == 0)
	if (buffer == 0)
	{
		printf("c %d -> %p @ *%p\n", num * size, buffer, __builtin_return_address(0));
		printf("h 0x%x (%d)\n", remaining, remaining);
		print_top_allocs();
	}
	else
	{
		struct metadata *worst = NULL;
		for (int i = 0; i < MAX_ALLOC_COUNT; ++i)
		{
			if (allocs[i].buffer_addr == NULL || allocs[i].size < worst->size)
				worst = allocs + i;
		}

		if (worst)
		{
			worst->buffer_addr = buffer;
			worst->size        = size;
			worst->return_addr = __builtin_return_address(0);
		}
	}
	return buffer;
}

void replace_pc(void *addr, void *ret_addr)
{
	for (int i = 0; i < MAX_ALLOC_COUNT; ++i)
	{
		if (addr && (addr == allocs[i].buffer_addr))
		{
			allocs[i].return_addr = ret_addr;
			break;
		}
	}
}

void *__wrap_mbedtls_calloc(size_t nmemb, size_t size)
{
	void *ret = __real_mbedtls_calloc(nmemb, size);
	replace_pc(ret, __builtin_return_address(0));
	return ret;
}
