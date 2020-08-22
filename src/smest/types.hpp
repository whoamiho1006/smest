#pragma once
#ifndef PLATFORM_WINDOWS
#	if defined(_WIN32) || defined(_WIN64)
#		define PLATFORM_WINDOWS 1
#	else
#		define PLATFORM_WINDOWS 0
#	endif
#endif

#define _CRT_SECURE_NO_WARNINGS 1

#include <stdint.h>
#include <string>
#include <vector>
#include <map>

namespace smest {
	/* Unreal Engine and Pure C++ both-way typedefs. */

#ifdef UPROPERTY
	typedef ::uint8		uint8;
	typedef ::uint16	uint16;
	typedef ::uint32	uint32;
	typedef ::uint64	uint64;

	typedef ::int8		int8;
	typedef ::int16		int16;
	typedef ::int32		int32;
	typedef ::int64		int64;
#else
	typedef ::uint8_t	uint8;
	typedef ::uint16_t	uint16;
	typedef ::uint32_t	uint32;
	typedef ::uint64_t	uint64;

	typedef ::int8_t	int8;
	typedef ::int16_t	int16;
	typedef ::int32_t	int32;
	typedef ::int64_t	int64;
#endif

	typedef std::string string;
}