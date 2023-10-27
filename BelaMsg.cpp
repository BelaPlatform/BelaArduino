#include <stdio.h>
#include <vector>
#include <array>
#include <stdint.h>
#include <string.h>
#include "BelaMsg.h"
#include "BelaArduino.h"

struct State {
	std::vector<uint8_t> msgBuf = std::vector<uint8_t>(1000);
	size_t msgDataPtr = 0;
	size_t msgHeaderPtr = 0;
};
static std::array<State,kBelaNumSourceThreads> states;

void msgInit(BelaSourceThread thread, BelaReceiver rec, size_t count)
{
	State& s = states[thread];
	s.msgDataPtr = count + kMsgPreHeader;
	s.msgBuf.resize(s.msgDataPtr);
	s.msgBuf[0] = count;
	s.msgBuf[1] = rec;
	s.msgHeaderPtr = kMsgPreHeader;
}

void msgPush(BelaSourceThread thread, char tag, const void* data, size_t size)
{
	State& s = states[thread];
	s.msgBuf[s.msgHeaderPtr++] = (uint8_t)tag;
	s.msgBuf.resize(s.msgDataPtr + size);
	memcpy(s.msgBuf.data() + s.msgDataPtr, data, size);
	s.msgDataPtr += size;
}

void msgAddFS(BelaSourceThread thread, const char* value)
{
	msgPush(thread, 's', value, strlen(value) + 1);
}

void msgSend(BelaSourceThread thread)
{
	State& s = states[thread];
	if(s.msgDataPtr != s.msgBuf.size())
	{
		fprintf(stderr, "ERROR: unexpected msgBuf size: %zu vs %zu\n", s.msgDataPtr, s.msgBuf.size());
	}
	bool error = false;
	bool rt = false;
	Pipe* pipe = nullptr;
	switch(thread)
	{
		case kBelaSourceThreadArduino:
			rt = false;
			pipe = &belaArduinoPipe;
			break;
		case kBelaSourceThreadAudio:
			rt = true;
			pipe = &belaArduinoPipe;
			break;
		case kBelaNumSourceThreads:
		default:
			break;
	}
	if(pipe)
	{
		if(rt)
		{
			error = !pipe->writeRt(s.msgBuf.size());
			error |= !pipe->writeRt(s.msgBuf.data(), s.msgBuf.size());
		} else {
			error = !pipe->writeNonRt(s.msgBuf.size());
			error |= !pipe->writeNonRt(s.msgBuf.data(), s.msgBuf.size());
		}
	} else
		error = true;
	if(error)
		fprintf(stderr, "Error writing to pipe from thread %d\n", thread);
}
