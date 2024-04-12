#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <assert.h>
#include <stdio.h>

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

namespace renderer::vulkan
{
	bool init();
	void shut_down();
	VkInstance get_vulkan_instance();
}
