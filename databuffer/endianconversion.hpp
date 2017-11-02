#pragma once
#include <cassert>
#include <algorithm>
#include <type_traits>

#if defined (__GLIBC__)
#   include <endian.h>
#endif

namespace endian
{

#if defined(_LITTLE_ENDIAN) \
    || ( defined(BYTE_ORDER) && defined(LITTLE_ENDIAN) && BYTE_ORDER == LITTLE_ENDIAN ) \
    || ( defined(_BYTE_ORDER) && defined(_LITTLE_ENDIAN) && _BYTE_ORDER == _LITTLE_ENDIAN ) \
    || ( defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && __BYTE_ORDER == __LITTLE_ENDIAN ) \
    || defined(__i386__) || defined(__alpha__) \
    || defined(__ia64) || defined(__ia64__) \
    || defined(_M_IX86) || defined(_M_IA64) \
    || defined(_M_ALPHA) || defined(__amd64) \
    || defined(__amd64__) || defined(_M_AMD64) \
    || defined(__x86_64) || defined(__x86_64__) \
    || defined(_M_X64)
#define SYSTEM_LITTLE_ENDIAN
#elif defined(_BIG_ENDIAN) \
    || ( defined(BYTE_ORDER) && defined(BIG_ENDIAN) && BYTE_ORDER == BIG_ENDIAN ) \
    || ( defined(_BYTE_ORDER) && defined(_BIG_ENDIAN) && _BYTE_ORDER == _BIG_ENDIAN ) \
    || ( defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && __BYTE_ORDER == __BIG_ENDIAN ) \
    || defined(__sparc) || defined(__sparc__) \
    || defined(_POWER) || defined(__powerpc__) \
    || defined(__ppc__) || defined(__hpux) \
    || defined(_MIPSEB) || defined(_POWER) \
    || defined(__s390__)
#define SYSTEM_BIG_ENDIAN
#else
#   error  Middle endian/NUXI order is not supported
#endif

	/*template<typename T>
	T Swap(T out)
	{
		static union autodetect
		{
			int word;
			char byte[sizeof(int)];
			autodetect() : word(1)
			{

#if defined SYSTEM_LITTLE_ENDIAN
				assert(("wrong endianness detected!", byte[0] == 1));
#elif defined SYSTEM_BIG_ENDIAN
				assert(("wrong endianness detected!", byte[0] == 0))
#endif
			}
		} _;

		if (!std::is_pod<T>::value)
		{
			return out;
		}

		char *ptr;

		switch (sizeof(T))
		{
		case 0:
		case 1:
			break;
		case 2:
			ptr = reinterpret_cast<char *>(&out);
			std::swap(ptr[0], ptr[1]);
			break;
		case 4:
			ptr = reinterpret_cast<char *>(&out);
			std::swap(ptr[0], ptr[3]);
			std::swap(ptr[1], ptr[2]);
			break;
		case 8:
			ptr = reinterpret_cast<char *>(&out);
			std::swap(ptr[0], ptr[7]);
			std::swap(ptr[1], ptr[6]);
			std::swap(ptr[2], ptr[5]);
			std::swap(ptr[3], ptr[4]);
			break;
		case 16:
			ptr = reinterpret_cast<char *>(&out);
			std::swap(ptr[0], ptr[15]);
			std::swap(ptr[1], ptr[14]);
			std::swap(ptr[2], ptr[13]);
			std::swap(ptr[3], ptr[12]);
			std::swap(ptr[4], ptr[11]);
			std::swap(ptr[5], ptr[10]);
			std::swap(ptr[6], ptr[9]);
			std::swap(ptr[7], ptr[8]);
			break;
		default:
			assert(!"POD type bigger than 256 bits (?)");
			break;
		}

		return out;
	}*/

	/*template<typename T>
	T LeToBe(const T &in)
	{
	return Swap(in);
	}
	template<typename T>
	T BeToLe(const T &in)
	{
	return Swap(in);
	}

	template<typename T>
	T LeToH(const T &in)
	{
	#if defined SYSTEM_LITTLE_ENDIAN
	return in;
	#elif defined SYSTEM_BIG_ENDIAN
	return Swap(in);
	#endif
	}
	template<typename T>
	T HToLe(const T &in)
	{
	#if defined SYSTEM_LITTLE_ENDIAN
	return in;
	#elif defined SYSTEM_BIG_ENDIAN
	return Swap(in);
	#endif
	}

	template<typename T>
	T BeToH(const T &in)
	{

	#if defined SYSTEM_LITTLE_ENDIAN
	return Swap(in);
	#elif defined SYSTEM_BIG_ENDIAN
	return in;
	#endif
	}
	template<typename T>
	T HToBe(const T &in)
	{
	#if defined SYSTEM_LITTLE_ENDIAN
	return Swap(in);
	#elif defined SYSTEM_BIG_ENDIAN
	return in;
	#endif
	}*/

	template<typename T>
	void Swap(T& out)
	{
		static union autodetect
		{
			int word;
			char byte[sizeof(int)];
			autodetect() : word(1)
			{

#if defined SYSTEM_LITTLE_ENDIAN
				assert(("wrong endianness detected!", byte[0] == 1));
#elif defined SYSTEM_BIG_ENDIAN
				assert(("wrong endianness detected!", byte[0] == 0))
#endif
			}
		} _;

		if (!std::is_pod<T>::value)
		{
			//return out;
			return;
		}

		char *ptr = NULL;

		switch (sizeof(T))
		{
		case 0:
		case 1:
			break;
		case 2:
			ptr = reinterpret_cast<char *>(&out);
			std::swap(ptr[0], ptr[1]);
			break;
		case 4:
			ptr = reinterpret_cast<char *>(&out);
			std::swap(ptr[0], ptr[3]);
			std::swap(ptr[1], ptr[2]);
			break;
		case 8:
			ptr = reinterpret_cast<char *>(&out);
			std::swap(ptr[0], ptr[7]);
			std::swap(ptr[1], ptr[6]);
			std::swap(ptr[2], ptr[5]);
			std::swap(ptr[3], ptr[4]);
			break;
		case 16:
			ptr = reinterpret_cast<char *>(&out);
			std::swap(ptr[0], ptr[15]);
			std::swap(ptr[1], ptr[14]);
			std::swap(ptr[2], ptr[13]);
			std::swap(ptr[3], ptr[12]);
			std::swap(ptr[4], ptr[11]);
			std::swap(ptr[5], ptr[10]);
			std::swap(ptr[6], ptr[9]);
			std::swap(ptr[7], ptr[8]);
			break;
		default:
			assert(!"POD type bigger than 256 bits (?)");
			break;
		}

		//return out;
	}



	template<typename T>
	void LeToBe(T& in)
	{
		Swap(in);
	}
	template<typename T>
	void BeToLe(T &in)
	{
		Swap(in);
	}

	template<typename T>
	void LeToH(T &in)
	{
#if defined SYSTEM_LITTLE_ENDIAN

#elif defined SYSTEM_BIG_ENDIAN
		Swap(in);
#endif
	}
	template<typename T>
	void HToLe(T &in)
	{
#if defined SYSTEM_LITTLE_ENDIAN

#elif defined SYSTEM_BIG_ENDIAN
		Swap(in);
#endif
	}

	template<typename T>
	void BeToH(T &in)
	{

#if defined SYSTEM_LITTLE_ENDIAN
		Swap(in);
#elif defined SYSTEM_BIG_ENDIAN
#endif
	}
	template<typename T>
	void HToBe(T &in)
	{
#if defined SYSTEM_LITTLE_ENDIAN
		Swap(in);
#elif defined SYSTEM_BIG_ENDIAN
		in;
#endif
	}
}

/*
以unsigned int value = 0x12345678为例，分别看看在两种字节序下其存储情况，我们可以用unsigned char buf[4]来表示value：

Big - Endian: 低地址存放高位，如下：

高地址

-------------- -

buf[3](0x78) --低位

buf[2](0x56)

buf[1](0x34)

buf[0](0x12) --高位

-------------- -

低地址

Little - Endian: 低地址存放低位，如下：

高地址

-------------- -

buf[3](0x12) --高位

buf[2](0x34)

buf[1](0x56)

buf[0](0x78) --低位

--------------

低地址

*/