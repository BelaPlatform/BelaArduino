#pragma once
#include <sys/types.h>
#include <string>

constexpr size_t kMsgPreHeader = 2;
enum BelaReceiver {
	kBelaReceiverShiftOut,
	kBelaReceiverPd = 123,
};

void msgInit(BelaReceiver, size_t);
void msgPush(char tag, const void* data, size_t size);
template <typename T>
void msgAdd(const T& t)
{
	// limit valid types to a subset where we are sure that the
	// typeid name is a single character
	static_assert(
		std::is_same<T,char>::value
		|| std::is_same<T,uint8_t>::value
		|| std::is_same<T,unsigned int>::value
		|| std::is_same<T,int>::value
		|| std::is_same<T,float>::value
		|| std::is_same<T,double>::value
		, "T is not of a supported type");
#if 0
	printf("char: %s\n", typeid(char).name()); // c
	printf("uint8_t: %s\n", typeid(uint8_t).name()); // h
	printf("unsigned int: %s\n", typeid(unsigned int).name()); // j
	printf("int: %s\n", typeid(int).name()); // i
	printf("float: %s\n", typeid(float).name()); // f
	printf("double: %s\n", typeid(double).name()); // d
#endif
	msgPush(*typeid(T).name(), &t, sizeof(T));
}
// msgAddFS only accepts float or "string"
static inline void msgAddFS(float f) { msgAdd(f); }
void msgAddFS(const char*);
static inline void msgAddFS(const std::string& s) { msgAddFS(s.c_str()); }
void msgSend();

static inline void msgAddT() {}; // termination case

template <typename T, typename... Ts>
void msgAddT(const T& arg, const Ts&... varargs)
{
	msgAddFS(arg);
	msgAddT(varargs...);
}

template <typename... Ts>
void belaMsgSend(BelaReceiver receiver, const Ts&... varargs)
{
	msgInit(receiver, sizeof...(varargs));
	msgAddT(varargs...);
	msgSend();
}
