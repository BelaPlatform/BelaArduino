#include <stdio.h>
#include <vector>
#include <stdint.h>
#include <string.h>
#include "BelaMsg.h"
#include "BelaArduino.h"

static std::vector<uint8_t> msgBuf(1000);
static size_t msgDataPtr;
static size_t msgHeaderPtr;

void msgInit(BelaReceiver rec, size_t count)
{
	msgDataPtr = count + kMsgPreHeader;
	msgBuf.resize(msgDataPtr);
	msgBuf[0] = count;
	msgBuf[1] = rec;
	msgHeaderPtr = kMsgPreHeader;
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
	msgPush('f', &value, sizeof(value));
}

void msgAdd(const char* value)
{
	msgPush('s', value, strlen(value) + 1);
}

void msgSend()
{
	if(msgDataPtr != msgBuf.size())
	{
		fprintf(stderr, "ERROR: unexpected msgBuf size: %zu vs %zu\n", msgDataPtr, msgBuf.size());
	}
	bool error = !belaArduinoPipe.writeNonRt(msgBuf.size());
	error |= !belaArduinoPipe.writeNonRt(msgBuf.data(), msgBuf.size());
	if(error)
		fprintf(stderr, "Error writing to belaArduinoPipe\n");
}
