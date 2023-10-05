#include <stdio.h>
#include <vector>
#include <stdint.h>
#include <string.h>
#include "BelaMsg.h"

static std::vector<uint8_t> msgBuf(1000);
static size_t msgDataPtr;
static size_t msgHeaderPtr;

void msgInit(size_t count)
{
	printf("msgInit: %zu\n", count);
	msgDataPtr = count + 1;
	msgBuf.resize(msgDataPtr);
	msgBuf[0] = count;
	msgHeaderPtr = 1;
}

static void msgPush(char tag, const void* data, size_t size)
{
	msgBuf[msgHeaderPtr++] = (uint8_t)tag;
	msgBuf.resize(msgDataPtr + size);
	memcpy(msgBuf.data() + msgDataPtr, data, size);
	msgDataPtr += size;
}

void msgAdd(float value)
{
	printf("msgAdd: %f\n", value);
	msgPush('f', &value, sizeof(value));
}

void msgAdd(const char* value)
{
	printf("msgAdd: %s\n", value);
	msgPush('s', value, strlen(value) + 1);
}

void msgSend()
{
	if(msgDataPtr != msgBuf.size())
	{
		fprintf(stderr, "ERROR: unexpected msgBuf size: %zu vs %zu\n", msgDataPtr, msgBuf.size());
	}
	for(size_t n = 0; n < msgDataPtr; ++n)
	{
		printf("[%2zu}\\%d '%c'\n", n, msgBuf[n], msgBuf[n]);
	}
}
