#pragma once
#include <sys/types.h>
#include <string>
#include <libraries/Pipe/Pipe.h>

constexpr size_t kMsgPreHeader = 2;
enum BelaSourceThread {
	kBelaSourceThreadArduino,
	kBelaSourceThreadAudio,
	kBelaNumSourceThreads,
};
enum BelaReceiver {
	kBelaReceiverShiftOut,
	kBelaReceiverPd = 123,
	kBelaReceiverArduino,
};

void msgInit(BelaSourceThread thread, BelaReceiver, size_t);
void msgPush(BelaSourceThread thread, char tag, const void* data, size_t size);
template <typename T>
void msgAdd(BelaSourceThread thread, const T& t)
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
	msgPush(thread, *typeid(T).name(), &t, sizeof(T));
}
// msgAddFS only accepts float or "string"
static inline void msgAddFS(BelaSourceThread thread, float f) { msgAdd(thread, f); }
void msgAddFS(BelaSourceThread, const char*);
static inline void msgAddFS(BelaSourceThread thread, const std::string& s)
{
	msgAddFS(thread, s.c_str());
}
void msgSend(BelaSourceThread thread);

static inline void msgAddT(BelaSourceThread) {}; // termination case

template <typename T, typename... Ts>
void msgAddT(BelaSourceThread thread, const T& arg, const Ts&... varargs)
{
	msgAddFS(thread, arg);
	msgAddT(thread, varargs...);
}

template <typename... Ts>
void belaMsgSend(BelaSourceThread thread, BelaReceiver receiver, const Ts&... varargs)
{
	msgInit(thread, receiver, sizeof...(varargs));
	msgAddT(thread, varargs...);
	msgSend(thread);
}

class BelaMsgParser {
public:
	struct Parsed {
		std::vector<uint8_t> msg = std::vector<uint8_t>(1000);
		BelaReceiver rec;
		const uint8_t* tags;
		uint8_t numTags;
		uint8_t nextArg;
		bool good = false;
	};
	BelaMsgParser(Pipe& pipe, bool rt) : pipe(pipe), rt(rt) {}
	Parsed& process()
	{
		p.good = false;
		if(kHeader == waitingFor)
		{
			// The header is a size_t containing the message size
			int ret = rt ? pipe.readRt(size) : pipe.readNonRt(size);
			if(1 == ret)
				waitingFor = kPayload;
			else
				return p;
		}
		if(kPayload == waitingFor)
		{
			// the message is:
			// 1 byte: n of tags
			// 1 byte: id of receiver
			// n tags
			// n arguments (without padding, possibly unaligned)
			p.msg.resize(size);
			size_t rd = rt ? pipe.readRt(p.msg.data(), size) : pipe.readNonRt(p.msg.data(), size);
			if(rd != size)
				return p;
			waitingFor = kHeader;
			p.good = true;
			p.numTags = p.msg[0];
			p.rec = BelaReceiver(p.msg[1]);
			p.tags = p.msg.data() + kMsgPreHeader;
			p.nextArg = kMsgPreHeader + p.numTags;
		}
		return p;
	}
private:
	enum WaitingFor {
		kHeader,
		kPayload
	} waitingFor = kHeader;
	size_t size = 0;
	Pipe& pipe;
	Parsed p;
	bool rt;
};
