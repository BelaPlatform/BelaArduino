#pragma once
#include <sys/types.h>

constexpr size_t kMsgPreHeader = 2;
enum BelaReceiver {
	kBelaReceiverPd = 123,
};

void msgInit(BelaReceiver, size_t);
void msgAdd(float);
void msgAdd(const char*);
void msgSend();

static inline void msgAddT() {}; // termination case

template <typename T, typename... Ts>
void msgAddT(const T& arg, const Ts&... varargs)
{
	msgAdd(arg);
	msgAddT(varargs...);
}

template <typename... Ts>
void belaMsgSend(BelaReceiver receiver, const Ts&... varargs)
{
	msgInit(receiver, sizeof...(varargs));
	msgAddT(varargs...);
	msgSend();
}
