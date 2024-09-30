#pragma once
#include "BelaMsg.h"

extern "C" {
#pragma weak pdReceiveMsg
void pdReceiveMsg(const char*, float*, unsigned int);
};

template <typename... Ts>
void pdSendMessage(const Ts&... varargs)
{
	belaMsgSend(kBelaSourceThreadArduino, kBelaReceiverPd, varargs...);
}
