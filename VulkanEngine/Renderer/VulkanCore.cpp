#include "VulkanCore.h"
#include <iostream>

#include <optional>

namespace renderer::vulkan
{
	namespace
	{
		VkInstance					vk_instance;
		// bool						enable_validation_layers;
		VkDebugUtilsMessengerEXT	debug_messenger;
		VkPhysicalDevice			device{ VK_NULL_HANDLE };
		VkDevice					logical_device;
		VkQueue						graphics_queue;

		struct
		{
			std::optional<uint32_t> graphics_family;

			bool is_complete()
			{
				return graphics_family.has_value();
			}
		}queue_family_indices;

		static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
															 VkDebugUtilsMessageTypeFlagsEXT message_type,
															 const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
															 void* user_data) 
		{
			if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
				std::cerr << "validation layer: " << callback_data->pMessage << std::endl;
			return VK_FALSE;
		}

		VkResult proxy_create_debug_utils_messenger_ext(const VkDebugUtilsMessengerCreateInfoEXT* create_info, 
														const VkAllocationCallbacks* allocator) 
		{
			assert(vk_instance);
			assert(!debug_messenger);
			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_instance, "vkCreateDebugUtilsMessengerEXT");
			if (func != nullptr) {
				return func(vk_instance, create_info, allocator, &debug_messenger);
			}
			else {
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		}

		void proxy_destroy_debug_utils_messenger_ext(const VkAllocationCallbacks* allocator) 
		{
			assert(vk_instance);
			assert(debug_messenger);
			auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_instance, "vkDestroyDebugUtilsMessengerEXT");
			if (func != nullptr) {
				func(vk_instance, debug_messenger, allocator);
			}
		}

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

		std::vector<const char*> get_required_extensions() {
			uint32_t glfw_extension_count = 0;
			const char** glfw_extensions;
			glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

			if (!check_glfw_extensions_support(glfw_extensions, glfw_extension_count))
			{
				std::cout << "glfw extensions differ from vulkan extensions!\n";
				return {};
			}

			std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

#if _DEBUG
				extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

			return extensions;
		}

		void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_dm_info)
		{
			create_dm_info = {};
			create_dm_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			create_dm_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
											 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
											 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			create_dm_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
										 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
										 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			create_dm_info.pfnUserCallback = debug_callback;
			create_dm_info.pUserData = nullptr; // Optional
		}

		bool is_device_suitable(const VkPhysicalDevice& device)
		{
			VkPhysicalDeviceProperties device_properties;
			vkGetPhysicalDeviceProperties(device, &device_properties);

			// checking support for optional features like texture compression, 
			// 64 bit floats and multi viewport rendering (useful for VR)
			VkPhysicalDeviceFeatures device_features;
			vkGetPhysicalDeviceFeatures(device, &device_features);

			uint32_t queue_family_count = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

			std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

			bool has_graphics_queue_family{ false };
			int i{ 0 };
			for (const auto& family : queue_families)
			{
				if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					has_graphics_queue_family = true;
					queue_family_indices.graphics_family = i;
					break;
				}
				++i;
			}

			return has_graphics_queue_family && 
				   device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && 
				   device_features.geometryShader;
		}

		void pick_physical_device()
		{
			assert(vk_instance);
			// get number of devices in system
			uint32_t device_count{ 0 };
			vkEnumeratePhysicalDevices(vk_instance, &device_count, nullptr);

			if (!device_count)
			{
				std::cout << "Found no vulkan suitable devices!\n";
				return;
			}

			assert(device_count);
			std::vector<VkPhysicalDevice> all_devices{ device_count };
			vkEnumeratePhysicalDevices(vk_instance, &device_count, all_devices.data());

			for (const auto& candidate : all_devices) {
				if (is_device_suitable(candidate)) {
					device = candidate;
					break;
				}
			}

			if (device == VK_NULL_HANDLE) {
				std::cout << "failed to find a suitable GPU!\n";
				return;
			}

		}

	}// anonymous namespace

	bool init()
	{
		VkApplicationInfo app_info{};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = "Hello Triangle";
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName = "No Engine";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &app_info;

#ifdef _DEBUG
		VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
		
		// setting validation layers
		const std::vector<const char*> validation_layers = {
			"VK_LAYER_KHRONOS_validation"
		};

		if (!check_validation_layers_support(validation_layers))
		{
			std::cout << "requested validation layer does not exist in supprted layers!\n";
			return false;
		}

		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();

		populate_debug_messenger_create_info(debug_create_info);
		create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
#else

		create_info.enabledLayerCount = 0;
		create_info.pNext = nullptr;

#endif

		// check extensions
		std::vector<const char*> extensions{ get_required_extensions() };
		assert(extensions.data());

		create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		create_info.ppEnabledExtensionNames = extensions.data();


		// creating vulkan instance
		VKCALL(vkCreateInstance(&create_info, nullptr, &vk_instance), "failed to create vulkan instance!");
		assert(vk_instance);
		if (!vk_instance) return false;

#if _DEBUG
		VkDebugUtilsMessengerCreateInfoEXT create_debug_messenger_info;
		populate_debug_messenger_create_info(create_debug_messenger_info);

		VKCALL(proxy_create_debug_utils_messenger_ext(&create_debug_messenger_info, nullptr), "failed to setup debug messenger!");
#endif

		pick_physical_device();

		// creating grpahics queue
		float graphics_queue_priority{ 1.f };
		assert(queue_family_indices.is_complete());
		VkDeviceQueueCreateInfo queue_create_info{};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_family_indices.graphics_family.value();
		queue_create_info.queueCount = 1; // for now we are only interested in a queue with graphics capabilities
		queue_create_info.pQueuePriorities = &graphics_queue_priority;

		// specifying used device features
		VkPhysicalDeviceFeatures device_features{}; // leave empty for now

		// create logical device
		VkDeviceCreateInfo logical_device_create_info{};
		logical_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		logical_device_create_info.pQueueCreateInfos = &queue_create_info;
		logical_device_create_info.queueCreateInfoCount = 1;
		logical_device_create_info.pEnabledFeatures = &device_features;

		// specifying device specific extensions and validation layers
		logical_device_create_info.enabledExtensionCount = 0;

		// in order to be compatible with older vulkan implementations, keep the distinction between 
		// instance and device specific validation layers
#ifdef _DEBUG
		logical_device_create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		logical_device_create_info.ppEnabledLayerNames = validation_layers.data();
#else
		logical_device_create_info.enabledLayerCount = 0;
#endif
		VKCALL(vkCreateDevice(device, &logical_device_create_info, nullptr, &logical_device), "failed to setup logical device!");
		assert(logical_device);
		if (!logical_device) return false;

		// retreive graphics queue handle
		vkGetDeviceQueue(logical_device, queue_family_indices.graphics_family.value(), 0, &graphics_queue);

		return true;
	}

	void shut_down()
	{
#if _DEBUG
		proxy_destroy_debug_utils_messenger_ext(nullptr);
#endif
		vkDestroyDevice(logical_device, nullptr);
		vkDestroyInstance(vk_instance, nullptr);
	}

	[[nodiscard]] VkInstance get_vulkan_instance() { return vk_instance; }
}