#pragma once

// skip definitions of min/max macros in windows.h
#ifndef NOMINMAX
#define NOMINMAX
#endif // !NOMINMAX

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <vector>
#include <array>
#include <optional>
#include <set>
#include <string>

#ifndef DISABLE_COPY
#define DISABLE_COPY(T)					      \
			explicit T(const T&) = delete;    \
			T& operator=(const T&) = delete;  
#endif // !DISABLE_COPY

#ifndef DISABLE_MOVE
#define DISABLE_MOVE(T) 			     \
			explicit T(T&&) = delete;    \
			T& operator=(T&&) = delete;  
#endif // !DISABLE_COPY

#ifndef DISABLE_COPY_AND_MOVE
#define DISABLE_COPY_AND_MOVE(T) DISABLE_COPY(T) DISABLE_MOVE(T)
#endif // !DISABLE_COPY

#if _DEBUG

#define VKCALL(x, text) \
if(x != VK_SUCCESS) { \
	char line_number[32];						\
	sprintf_s(line_number, "%u", __LINE__);		\
	printf("error: %s, in file %s In line %s, trying %s.\n", #text, __FILE__, line_number, #x);		\
	__debugbreak();								\
}

#else

#define VKCALL(x, text) x

#endif

