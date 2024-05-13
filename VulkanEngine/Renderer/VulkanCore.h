#pragma once
#include "VulkanCommonHeaders.h"

namespace renderer::vulkan::core
{
	constexpr int max_current_frames{ 3 };

	bool init(GLFWwindow* window);
	void shutdown();

	VkInstance get_vulkan_instance();
	VkPhysicalDevice get_physical_device();
	VkPhysicalDeviceProperties	get_physical_device_properties();
	VkDevice get_logical_device();

	VkQueue get_graphics_queue();
	uint32_t get_graphics_queue_family_index();
	VkQueue get_present_queue();
	uint32_t get_present_queue_family_index();
	VkQueue get_transfer_queue();
	uint32_t get_transfer_queue_family_index();

	uint32_t get_current_command_buffer_index();
	VkCommandBuffer get_command_buffer();
	VkFramebuffer get_frame_buffer();
	VkExtent2D get_swap_chain_extent();
	float get_extent_aspect_ratio();
	VkFormat get_swap_chain_image_format();
	VkFormat get_swap_chain_depth_format();

	bool create_frame_buffers(VkRenderPass render_pass);
	void begin_frame();
	void end_frame();

	void frame_buffer_resize_callback(GLFWwindow* window, int width, int height);
	void transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout);
	
}
