#include "VulkanHelpers.h"

namespace renderer::vulkan::vkh
{
	namespace
	{

	} // anonymous namespace

	void print_available_extensions(const std::vector<VkExtensionProperties>& extensions)
	{
		std::cout << "available extensions:\n";

		for (const auto& extension : extensions) {
			std::cout << '\t' << extension.extensionName << '\n';
		}
	}

	bool check_glfw_extensions_support(const char** glfw_extensions, const uint32_t glfw_extension_count)
	{
		uint32_t extension_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
		std::vector<VkExtensionProperties> vk_extensions{extension_count};
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, vk_extensions.data());

		print_available_extensions(vk_extensions);

		for (uint32_t i{ 0 }; i < glfw_extension_count; ++i)
		{
			bool found{ false };
			for (const auto& vk_extension : vk_extensions)
			{
				if (strcmp(glfw_extensions[i], vk_extension.extensionName) == 0)
				{
					found = true;
					break;
				}
			}
			if (!found)
				return false;
		}
		return true;
	}

	bool check_validation_layers_support(const std::vector<const char*>& requested_layers)
	{
		uint32_t layer_count;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

		std::vector<VkLayerProperties> available_layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

		for (const char* layer_name : requested_layers)
		{
			bool found{ false };

			for (const auto& layer : available_layers) {
				if (strcmp(layer_name, layer.layerName) == 0) {
					found = true;
					break;
				}
			}

			if (!found) {
				return false;
			}
		}

		return true;
	}

	bool check_device_extensions_support(const VkPhysicalDevice& device, const std::vector<const char*>& device_extensions)
	{
		uint32_t extension_count;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

		std::vector<VkExtensionProperties> available_extensions(extension_count);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

		std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

		for (const auto& extension : available_extensions) {
			required_extensions.erase(extension.extensionName);
		}

		return required_extensions.empty();
	}

	std::vector<const char*> get_required_extensions() 
	{
		uint32_t glfw_extension_count = 0;
		const char** glfw_extensions;
		glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

		if (!check_glfw_extensions_support(glfw_extensions, glfw_extension_count))
		{
			std::cout << "glfw extensions are not supported!\n";
			return {};
		}

		std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

#if _DEBUG
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

		return extensions;
	}


	VkFormat find_supported_format(VkPhysicalDevice device, 
									const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(device, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) 
			{
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) 
			{
				return format;
			}
		}

		throw std::runtime_error("failed to find supported format!");
		
	}

}