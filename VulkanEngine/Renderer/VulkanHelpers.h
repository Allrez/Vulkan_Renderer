#pragma once
#include "VulkanCore.h"

namespace renderer::vulkan::vkh
{

	inline VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool command_pool,
																	VkCommandBufferLevel level,
																	uint32_t buffer_count)
	{
		VkCommandBufferAllocateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.level = level;
		info.commandPool = command_pool;
		info.commandBufferCount = buffer_count;
		return info;
	}

	inline VkCommandBufferBeginInfo command_buffer_begin_info()
	{
		VkCommandBufferBeginInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		return info;
	}

	inline VkCommandPoolCreateInfo command_pool_create_info(VkCommandPoolCreateFlags flags, uint32_t queue_family_index)
	{
		VkCommandPoolCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.flags = flags;
		info.queueFamilyIndex = queue_family_index;
		return info;
	}

	inline VkViewport viewport(float width, float height, float min_depth, float max_depth)
	{
		VkViewport viewport{};
		viewport.width = width;
		viewport.height = height;
		viewport.minDepth = min_depth;
		viewport.maxDepth = max_depth;
		return viewport;
	}

	inline VkRect2D rect_2d(uint32_t width, uint32_t height, uint32_t offset_x, uint32_t offset_y)
	{
		VkRect2D rect{};
		rect.extent.width = width;
		rect.extent.height = height;
		rect.offset.x = offset_x;
		rect.offset.y = offset_y;
		return rect;
	}

	inline VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state(VkPrimitiveTopology topology,
																				VkPipelineInputAssemblyStateCreateFlags flags,
																				VkBool32 primitive_restart_enable)
	{
		VkPipelineInputAssemblyStateCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		info.topology = topology;
		info.flags = flags;
		info.primitiveRestartEnable = primitive_restart_enable;
		return info;
	}

	inline VkDescriptorPoolCreateInfo descriptor_pool(uint32_t pool_size_count, 
													  VkDescriptorPoolSize* pool_sizes, uint32_t max_sets)
	{
		VkDescriptorPoolCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.poolSizeCount = pool_size_count;
		info.pPoolSizes = pool_sizes;
		info.maxSets = max_sets;
		return info;
	}

	inline VkDescriptorSetLayoutBinding descriptor_set_layout_binding(VkDescriptorType type,
																	  VkShaderStageFlags stage_flags,
																	  uint32_t binding,
																	  uint32_t descriptor_count = 1)
	{
		VkDescriptorSetLayoutBinding layout_binding{};
		layout_binding.binding = binding;
		layout_binding.descriptorType = type;
		layout_binding.descriptorCount = descriptor_count;
		layout_binding.stageFlags = stage_flags;
		return layout_binding;
	}

	inline VkDescriptorSetLayoutCreateInfo descriptor_set_layout(uint32_t binding_count, const VkDescriptorSetLayoutBinding* bindings)
	{
		VkDescriptorSetLayoutCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.bindingCount = binding_count;
		info.pBindings = bindings;
		return info;
	}

	inline VkDescriptorSetAllocateInfo descriptor_set_alloc_info(VkDescriptorPool descriptor_pool,
																 const VkDescriptorSetLayout* set_layouts,
																 uint32_t descriptor_set_count)
	{
		VkDescriptorSetAllocateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.descriptorPool = descriptor_pool;
		info.descriptorSetCount = descriptor_set_count;
		info.pSetLayouts = set_layouts;
		return info;
	}

	inline VkWriteDescriptorSet write_descriptor_set(VkDescriptorSet dst_set, VkDescriptorType type, uint32_t binding,
													 VkDescriptorBufferInfo* buffer_info, uint32_t descriptor_count = 1)
	{
		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = dst_set;
		write.descriptorType = type;
		write.dstBinding = binding;
		write.pBufferInfo = buffer_info;
		write.descriptorCount = descriptor_count;
		return write;
	}

	inline VkWriteDescriptorSet write_descriptor_set(VkDescriptorSet dst_set, VkDescriptorType type, uint32_t binding,
													 VkDescriptorImageInfo* image_info, uint32_t descriptor_count = 1)
	{
		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = dst_set;
		write.descriptorType = type;
		write.dstBinding = binding;
		write.pImageInfo = image_info;
		write.descriptorCount = descriptor_count;
		return write;
	}

	inline VkPipelineShaderStageCreateInfo pipeline_shader_stage(VkShaderModule module, VkShaderStageFlagBits stages, const char* name)
	{
		VkPipelineShaderStageCreateInfo shader_stage{};
		shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stage.module = module;
		shader_stage.stage = stages;
		shader_stage.pName = name;
		return shader_stage;
	}

	inline VkPipelineDynamicStateCreateInfo pipeline_dynamic_state(const std::vector<VkDynamicState>& dynamic_states)
	{
		VkPipelineDynamicStateCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		info.dynamicStateCount = (uint32_t)dynamic_states.size();
		info.pDynamicStates = dynamic_states.data();
		return info;
	}

	inline VkPipelineViewportStateCreateInfo viewport_state_dynamic(uint32_t viewport_count = 1, uint32_t scissor_count = 1)
	{
		VkPipelineViewportStateCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		info.viewportCount = viewport_count;
		info.scissorCount = scissor_count;
		return info;
	}

	inline VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state(
		VkPolygonMode polygon_mode,
		VkCullModeFlags cull_mode,
		VkFrontFace front_face,
		VkPipelineRasterizationStateCreateFlags flags = 0)
	{
		VkPipelineRasterizationStateCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		info.polygonMode = polygon_mode;
		info.cullMode = cull_mode;
		info.frontFace = front_face;
		info.flags = flags;
		info.depthClampEnable = VK_FALSE;
		info.lineWidth = 1.0f;
		return info;
	}

	inline VkPipelineMultisampleStateCreateInfo pipeline_multisample_state()
	{
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; 
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;
		return multisampling;
	}
	
	inline VkBufferCreateInfo buffer(VkDeviceSize size, VkSharingMode sharing_mode, VkBufferUsageFlags usage)
	{
		VkBufferCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.size = size;
		info.sharingMode = sharing_mode;
		info.usage = usage;
		return info;
	}

	inline VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state(
		VkColorComponentFlags color_write_mask,
		VkBool32 blend_enable)
	{
		VkPipelineColorBlendAttachmentState attachment_state{};
		attachment_state.colorWriteMask = color_write_mask;
		attachment_state.blendEnable = blend_enable;
		attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
		attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
		return attachment_state;
	}

	inline VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state(
		uint32_t attachment_count,
		const VkPipelineColorBlendAttachmentState* attachments)
	{
		VkPipelineColorBlendStateCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		info.attachmentCount = attachment_count;
		info.pAttachments = attachments;
		info.logicOpEnable = VK_FALSE;
		info.logicOp = VK_LOGIC_OP_COPY;
		info.blendConstants[0] = 0.0f;
		info.blendConstants[1] = 0.0f;
		info.blendConstants[2] = 0.0f;
		info.blendConstants[3] = 0.0f;
		return info;
	}

	inline VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state(
		VkBool32 depth_test_enable,
		VkBool32 depth_write_enable,
		VkCompareOp depth_compare_op)
	{
		VkPipelineDepthStencilStateCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		info.depthTestEnable = depth_test_enable;
		info.depthWriteEnable = depth_write_enable;
		info.depthCompareOp = depth_compare_op;
		info.back.compareOp = VK_COMPARE_OP_ALWAYS;
		return info;
	}

	void print_available_extensions(const std::vector<VkExtensionProperties>& extensions);
	bool check_glfw_extensions_support(const char** glfw_extensions, const uint32_t glfw_extension_count);
	bool check_validation_layers_support(const std::vector<const char*>& requested_layers);
	bool check_device_extensions_support(const VkPhysicalDevice& device, const std::vector<const char*>& device_extensions);
	std::vector<const char*> get_required_extensions();

	VkFormat find_supported_format(VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

}
