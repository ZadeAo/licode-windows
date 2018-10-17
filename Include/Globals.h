
// Globals.h

// This file gets included from every module in the project, so that global symbols may be introduced easily
// Also used for precompiled header generation in MSVC environments





// Compiler-dependent stuff:
#if defined(_MSC_VER)
	// Disable some warnings that we don't care about:
	#pragma warning(disable:4100)  // Unreferenced formal parameter

	// Useful warnings from warning level 4:
	#pragma warning(3 : 4189)  // Local variable is initialized but not referenced
	#pragma warning(3 : 4245)  // Conversion from 'type1' to 'type2', signed / unsigned mismatch
	#pragma warning(3 : 4310)  // Cast truncates constant value
	#pragma warning(3 : 4389)  // Signed / unsigned mismatch
	#pragma warning(3 : 4505)  // Unreferenced local function has been removed
	#pragma warning(3 : 4701)  // Potentially unitialized local variable used
	#pragma warning(3 : 4702)  // Unreachable code
	#pragma warning(3 : 4706)  // Assignment within conditional expression
	#pragma warning(disable : 4251)
	#pragma warning(disable : 4996)
	// 2014-10-23 xoft: Disabled this because the new C++11 headers in MSVC produce tons of these warnings uselessly
	// #pragma warning(3 : 4127)  // Conditional expression is constant

	// Disabling this warning, because we know what we're doing when we're doing this:
	#pragma warning(disable: 4355)  // 'this' used in initializer list

	// Disabled because it's useless:
	#pragma warning(disable: 4512)  // 'class': assignment operator could not be generated - reported for each class that has a reference-type member
	#pragma warning(disable: 4351)  // new behavior: elements of array 'member' will be default initialized
	
	// 2014_01_06 xoft: Disabled this warning because MSVC is stupid and reports it in obviously wrong places
	// #pragma warning(3 : 4244)  // Conversion from 'type1' to 'type2', possible loss of data

	#define OBSOLETE __declspec(deprecated)

	// No alignment needed in MSVC
	#define ALIGN_8
	#define ALIGN_16
	
	#define FORMATSTRING(formatIndex, va_argsIndex)

	// MSVC has its own custom version of zu format
	#define SIZE_T_FMT "%Iu"
	#define SIZE_T_FMT_PRECISION(x) "%" #x "Iu"
	#define SIZE_T_FMT_HEX "%Ix"
	
	#define NORETURN      __declspec(noreturn)

	// Use non-standard defines in <cmath>
	#define _USE_MATH_DEFINES

#elif defined(__GNUC__)

	// TODO: Can GCC explicitly mark classes as abstract (no instances can be created)?
	#define abstract

	// override is part of c++11
	#if __cplusplus < 201103L
		#define override
	#endif

	#define OBSOLETE __attribute__((deprecated))

	#define ALIGN_8 __attribute__((aligned(8)))
	#define ALIGN_16 __attribute__((aligned(16)))

	// Some portability macros :)
	#define stricmp strcasecmp
	
	#define FORMATSTRING(formatIndex, va_argsIndex) __attribute__((format (printf, formatIndex, va_argsIndex)))

	#if defined(_WIN32)
		// We're compiling on MinGW, which uses an old MSVCRT library that has no support for size_t printfing.
		// We need direct size formats:
		#if defined(_WIN64)
			#define SIZE_T_FMT "%I64u"
			#define SIZE_T_FMT_PRECISION(x) "%" #x "I64u"
			#define SIZE_T_FMT_HEX "%I64x"
		#else
			#define SIZE_T_FMT "%u"
			#define SIZE_T_FMT_PRECISION(x) "%" #x "u"
			#define SIZE_T_FMT_HEX "%x"
		#endif
	#else
		// We're compiling on Linux, so we can use libc's size_t printf format:
		#define SIZE_T_FMT "%zu"
		#define SIZE_T_FMT_PRECISION(x) "%" #x "zu"
		#define SIZE_T_FMT_HEX "%zx"
	#endif
	
	#define NORETURN      __attribute((__noreturn__))

#else

	#error "You are using an unsupported compiler, you might need to #define some stuff here for your compiler"

	/*
	// Copy and uncomment this into another #elif section based on your compiler identification

	// Explicitly mark classes as abstract (no instances can be created)
	#define abstract

	// Mark virtual methods as overriding (forcing them to have a virtual function of the same signature in the base class)
	#define override

	// Mark functions as obsolete, so that their usage results in a compile-time warning
	#define OBSOLETE

	// Mark types / variables for alignment. Do the platforms need it?
	#define ALIGN_8
	#define ALIGN_16
	*/

#endif


#ifdef  _DEBUG
	#define NORETURNDEBUG NORETURN
#else
	#define NORETURNDEBUG
#endif


#include <stddef.h>


// Integral types with predefined sizes:
typedef signed long long Int64;
typedef signed int       Int32;
typedef signed short     Int16;
typedef signed char      Int8;

typedef unsigned long long UInt64;
typedef unsigned int       UInt32;
typedef unsigned short     UInt16;
typedef unsigned char      UInt8;

typedef unsigned char Byte;
typedef Byte ColourID;


// If you get an error about specialization check the size of integral types
template <typename T, size_t Size, bool x = sizeof(T) == Size>
class SizeChecker;

template <typename T, size_t Size>
class SizeChecker<T, Size, true>
{
	T v;
};

template class SizeChecker<Int64, 8>;
template class SizeChecker<Int32, 4>;
template class SizeChecker<Int16, 2>;
template class SizeChecker<Int8,  1>;

template class SizeChecker<UInt64, 8>;
template class SizeChecker<UInt32, 4>;
template class SizeChecker<UInt16, 2>;
template class SizeChecker<UInt8,  1>;

// A macro to disallow the copy constructor and operator = functions
// This should be used in the private: declarations for any class that shouldn't allow copying itself
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
	TypeName(const TypeName &); \
	void operator =(const TypeName &)

// A macro that is used to mark unused local variables, to avoid pedantic warnings in gcc / clang / MSVC
// Note that in MSVC it requires the full type of X to be known
#define UNUSED_VAR(X) (void)(X)

// A macro that is used to mark unused function parameters, to avoid pedantic warnings in gcc
// Written so that the full type of param needn't be known
#ifdef _MSC_VER
	#define UNUSED(X)
#else
	#define UNUSED UNUSED_VAR
#endif




// OS-dependent stuff:
#ifdef _WIN32

	#define WIN32_LEAN_AND_MEAN
	#define _WIN32_WINNT _WIN32_WINNT_WS03  // We want to target Windows XP with Service Pack 2 & Windows Server 2003 with Service Pack 1 and higher

	// Windows SDK defines min and max macros, messing up with our std::min and std::max usage
	#define NOMINMAX

	#include <Windows.h>
	#include <winsock2.h>
	#include <Ws2tcpip.h>  // IPv6 stuff

	// Windows SDK defines GetFreeSpace as a constant, probably a Win16 API remnant
	#ifdef GetFreeSpace
		#undef GetFreeSpace
	#endif  // GetFreeSpace
#else
	#include <sys/types.h>
	#include <sys/time.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <time.h>
	#include <dirent.h>
	#include <errno.h>
	#include <iostream>
	#include <cstdio>
	#include <cstring>
	#include <pthread.h>
	#include <semaphore.h>
	#include <errno.h>
	#include <fcntl.h>
	#include <unistd.h>
#endif

#if defined(ANDROID_NDK)
	#define FILE_IO_PREFIX "/sdcard/Cuberite/"
#else
	#define FILE_IO_PREFIX ""
#endif





// CRT stuff:
#include <sys/stat.h>
#include <cassert>
#include <cstdio>
#include <cmath>
#include <cstdarg>





// STL stuff:
#include <array>
#include <chrono>
#include <vector>
#include <list>
#include <deque>
#include <string>
#include <map>
#include <algorithm>
#include <memory>
#include <set>
#include <queue>
#include <limits>
#include <chrono>
#include <mutex>
#include <thread>





 // a tick is 50 ms
 using cTickTime = std::chrono::duration < int, std::ratio_multiply<std::chrono::milliseconds::period, std::ratio<50>> > ;
 using cTickTimeLong = std::chrono::duration < Int64, cTickTime::period > ;
// Common headers (part 2, with macros):





