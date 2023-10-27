#pragma once
#include <BelaMsg.h>

template <typename... Ts>
void pdSendMessage(const Ts&... varargs)
{
	belaMsgSend(kBelaReceiverPd, varargs...);
}
