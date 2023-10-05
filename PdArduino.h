#pragma once
#include <sys/types.h>
#include <BelaMsg.h>

static inline void msgAddT() {}; // termination case

template <typename T, typename... Ts>
void msgAddT(const T& arg, const Ts&... varargs)
{
	msgAdd(arg);
	msgAddT(varargs...);
}

template <typename... Ts>
void pdSendMessage(const Ts&... varargs)
{
	msgInit(sizeof...(varargs));
	msgAddT(varargs...);
	msgSend();
}
