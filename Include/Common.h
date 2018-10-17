// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 COMMON_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// MEDIA_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。

#pragma once

#ifdef COMMON_EXPORTS
#define COMMON_API __declspec(dllexport)
#else
#define COMMON_API __declspec(dllimport)
#endif


// Common headers (part 1, without macros):
#include "StringUtils.h"
#include "OSSupport/CriticalSection.h"
#include "OSSupport/Event.h"
#include "OSSupport/File.h"
#include "OSSupport/StackTrace.h"
#include "OSSupport/IsThread.h"

// These fiunctions are defined in Logger.cpp, but are declared here to avoid including all of logger.h
COMMON_API void LOG(const char * a_Format, ...) FORMATSTRING(1, 2);
COMMON_API void LOGINFO(const char * a_Format, ...) FORMATSTRING(1, 2);
COMMON_API void LOGWARNING(const char * a_Format, ...) FORMATSTRING(1, 2);
COMMON_API void LOGERROR(const char * a_Format, ...) FORMATSTRING(1, 2);





// In debug builds, translate LOGD to LOG, otherwise leave it out altogether:
#ifdef _DEBUG
#define LOGD LOG
#else
#define LOGD(...)
#endif  // _DEBUG

#define LOGWARN LOGWARNING






// Common definitions:

/** Evaluates to the number of elements in an array (compile-time!) */
#define ARRAYCOUNT(X) (sizeof(X) / sizeof(*(X)))

/** Allows arithmetic expressions like "32 KiB" (but consider using parenthesis around it, "(32 KiB)") */
#define KiB * 1024
#define MiB * 1024 * 1024

/** Faster than (int)floorf((float)x / (float)div) */
#define FAST_FLOOR_DIV(x, div) (((x) - (((x) < 0) ? ((div) - 1) : 0)) / (div))

// Own version of assert() that writes failed assertions to the log for review
#ifdef  _DEBUG
#define ASSERT( x) ( !!(x) || ( LOGERROR("Assertion failed: %s, file %s, line %i", #x, __FILE__, __LINE__), PrintStackTrace(), assert(0), 0))
#else
#define ASSERT(x) ((void)(x))
#endif

// Pretty much the same as ASSERT() but stays in Release builds
#define VERIFY( x) ( !!(x) || ( LOGERROR("Verification failed: %s, file %s, line %i", #x, __FILE__, __LINE__), PrintStackTrace(), exit(1), 0))

//temporary replacement for std::make_unique until we get c++14

namespace cpp14
{
	template <class T, class... Args>
	std::unique_ptr<T> make_unique(Args&&... args)
	{
		return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
	}
}