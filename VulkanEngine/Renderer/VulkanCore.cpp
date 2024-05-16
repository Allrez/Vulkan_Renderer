
#include "VulkanCore.h"
#include "VulkanHelpers.h"
#include "VulkanSurface.h"
#include "VulkanResource.h"
#include "VulkanDescriptors.h"
#include "VulkanMemory.h"

namespace renderer::vulkan::core
{
	namespace
	{
		struct
		{
			std::optional<uint32_t> graphics_family;
			std::optional<uint32_t> present_family;
			std::optional<uint32_t> transfer_family;

			bool is_complete()
			{
				return graphics_family.has_value() && present_family.has_value() && transfer_family.has_value();
			}
		}queue_family_indices{};

		class vulkan_command
		{
		public:
			explicit vulkan_command() = default;
			DISABLE_COPY_AND_MOVE(vulkan_command);

			bool create_command_pool()
			{
				VkCommandPoolCreateInfo pool_info = vkh::command_pool_create_info(
					VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queue_family_indices.present_family.value());

				VKCALL(vkCreateCommandPool(get_logical_device(), &pool_info, nullptr, &_command_pool), "failed to create command pool");
				assert(_command_pool);
				if (!_command_pool)
					return false;

				return true;
			}

			bool create_command_buffer()
			{
				_command_buffers.resize(max_current_frames);

				VkCommandBufferAllocateInfo alloc_info = vkh::command_buffer_allocate_info(_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, max_current_frames);

				VKCALL(vkAllocateCommandBuffers(get_logical_device(), &alloc_info, _command_buffers.data()), "failed to allocate command buffers!");
				assert(_command_buffers.data());
				if (!_command_buffers.data())
					return false;

				_image_available_semaphores.resize(max_current_frames);
				_render_finished_semaphores.resize(max_current_frames);
				_fences.resize(max_current_frames);

				VkSemaphoreCreateInfo semaphore_info{};
				semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

				VkFenceCreateInfo fence_info{};
				fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // used for the first frame

				for (size_t i{ 0 }; i < max_current_frames; ++i)
				{
					VKCALL(vkCreateSemaphore(get_logical_device(), &semaphore_info, nullptr, &_image_available_semaphores[i]), "failed to create image available semaphore");

					VKCALL(vkCreateSemaphore(get_logical_device(), &semaphore_info, nullptr, &_render_finished_semaphores[i]), "failed to create render finished semaphore");

					VKCALL(vkCreateFence(get_logical_device(), &fence_info, nullptr, &_fences[i]), "failed to create fence");

					assert(_image_available_semaphores[i] && _render_finished_semaphores[i] && _fences[i]);
					if (!(_image_available_semaphores[i] && _render_finished_semaphores[i] && _fences[i]))
						return false;
				}

				return true;
			}

			void destroy()
			{
				for (size_t i{ 0 }; i < max_current_frames; ++i)
				{
					vkDestroySemaphore(get_logical_device(), _image_available_semaphores[i], nullptr);
					vkDestroySemaphore(get_logical_device(), _render_finished_semaphores[i], nullptr);
					vkDestroyFence(get_logical_device(), _fences[i], nullptr);
				}

				vkDestroyCommandPool(get_logical_device(), _command_pool, nullptr);
			}

			void begin_frame(vulkan_surface* vk_surface)
			{
				VkDevice logical_device{ get_logical_device() };
				VkExtent2D swap_chain_extent{ vk_surface->get_swap_chain_extent() };

				vkWaitForFences(logical_device, 1, &_fences[_current_frame], VK_TRUE, UINT64_MAX);

				VkResult result = vkAcquireNextImageKHR(logical_device, vk_surface->get_swap_chain(), UINT64_MAX,
					_image_available_semaphores[_current_frame], VK_NULL_HANDLE, &_image_index);

				if (result == VK_ERROR_OUT_OF_DATE_KHR)
				{
					vk_surface->recreate_swap_chain(queue_family_indices.graphics_family.value(),
						queue_family_indices.present_family.value());
					return;
				}
				else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
				{
					throw std::runtime_error("failed to acquire swap chain image!");
				}

				// only reset fence if we are submitting work
				vkResetFences(logical_device, 1, &_fences[_current_frame]);
				vkResetCommandBuffer(_command_buffers[_current_frame], 0);

				VkCommandBufferBeginInfo begin_info = vkh::command_buffer_begin_info();
				VKCALL(vkBeginCommandBuffer(_command_buffers[_current_frame], &begin_info), "failed to reset recording command buffer");
			}

			void end_frame(vulkan_surface* vk_surface, VkQueue graphics_queue, VkQueue present_queue)
			{
				// Submit the recorded command buffer
				VkSubmitInfo submit_info{};
				submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

				// which semaphores to wait on before execution begins
				VkSemaphore wait_semaphores[] = { _image_available_semaphores[_current_frame] };
				// in which stages of the pipeline to wait
				VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

				submit_info.waitSemaphoreCount = 1;
				submit_info.pWaitSemaphores = wait_semaphores;
				submit_info.pWaitDstStageMask = wait_stages;
				submit_info.commandBufferCount = 1;
				submit_info.pCommandBuffers = &_command_buffers[_current_frame];

				VkSemaphore signal_semaphores[] = { _render_finished_semaphores[_current_frame] };
				submit_info.signalSemaphoreCount = 1;
				submit_info.pSignalSemaphores = signal_semaphores;

				VKCALL(vkQueueSubmit(graphics_queue, 1, &submit_info, _fences[_current_frame]), "failed to submit draw command buffer!");

				VkPresentInfoKHR present_info{};
				present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
				present_info.waitSemaphoreCount = 1;
				present_info.pWaitSemaphores = signal_semaphores;

				VkSwapchainKHR swap_chains[] = { vk_surface->get_swap_chain() };
				present_info.swapchainCount = 1;
				present_info.pSwapchains = swap_chains;
				present_info.pImageIndices = &_image_index;
				present_info.pResults = nullptr; // Optional

				VkResult result = vkQueuePresentKHR(present_queue, &present_info);

				if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _frame_buffer_resized)
				{
					_frame_buffer_resized = false;
					vk_surface->recreate_swap_chain(queue_family_indices.graphics_family.value(),
						queue_family_indices.present_family.value());
				}
				else if (result != VK_SUCCESS) {
					throw std::runtime_error("failed to present swap chain image!");
				}

				_current_frame = (_current_frame + 1) % max_current_frames;
			}

			VkCommandBuffer begin_single_time_commands()
			{
				VkCommandBufferAllocateInfo info{};
				info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				info.commandPool = _command_pool;
				info.commandBufferCount = 1;

				VkCommandBuffer command_buffer;
				vkAllocateCommandBuffers(core::get_logical_device(), &info, &command_buffer);

				VkCommandBufferBeginInfo begin_info{};
				begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

				vkBeginCommandBuffer(command_buffer, &begin_info);
				return command_buffer;
			}

			void end_single_time_commands(VkCommandBuffer command_buffer, VkQueue graphics_queue)
			{
				vkEndCommandBuffer(command_buffer);

				VkSubmitInfo submit_info{};
				submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submit_info.commandBufferCount = 1;
				submit_info.pCommandBuffers = &command_buffer;

				vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
				vkQueueWaitIdle(graphics_queue);

				vkFreeCommandBuffers(core::get_logical_device(), _command_pool, 1, &command_buffer);

			}

			void transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, VkQueue graphics_queue)
			{
				VkCommandBuffer command_buffer = begin_single_time_commands();

				VkImageMemoryBarrier barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.oldLayout = old_layout;
				barrier.newLayout = new_layout;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.image = image;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;


				VkPipelineStageFlags source_stage;
				VkPipelineStageFlags destination_stage;

				if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				{
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

					source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
					destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				}
				else
				{
					throw std::invalid_argument("unsupported layout transition!");
				}


				vkCmdPipelineBarrier(
					command_buffer,
					source_stage, destination_stage,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier
				);

				end_single_time_commands(command_buffer, graphics_queue);
			}


			[[nodiscard]] constexpr VkCommandPool get_command_pool() { return _command_pool; }
			[[nodiscard]] constexpr uint32_t get_current_command_buffer_index() { return _current_frame; }
			[[nodiscard]] constexpr uint32_t get_current_image_index() { return _image_index; }
			constexpr void set_frame_buffer_resized() { _frame_buffer_resized = true; }
			[[nodiscard]] VkCommandBuffer get_command_buffer() { return _command_buffers[_current_frame]; }

		private:
			VkCommandPool						_command_pool{ nullptr };
			std::vector<VkCommandBuffer>		_command_buffers{ nullptr };
			std::vector<VkSemaphore>			_image_available_semaphores{ nullptr };
			std::vector<VkSemaphore>			_render_finished_semaphores{ nullptr };
			std::vector<VkFence>				_fences{ nullptr };
			uint32_t							_current_frame{ 0 };
			uint32_t							_image_index{ 0 };
			bool								_frame_buffer_resized{ false };
		};

		VkInstance					instance{ nullptr };
		VkDebugUtilsMessengerEXT	debug_messenger{ nullptr };
		vulkan_surface				vk_surface{};
		vulkan_command				vk_command{};
		
		VkPhysicalDevice			device{ VK_NULL_HANDLE };
		VkPhysicalDeviceFeatures	device_features;
		VkPhysicalDeviceProperties	device_properties;

		VkDevice					logical_device{ VK_NULL_HANDLE };
		VkQueue						graphics_queue{ VK_NULL_HANDLE };
		VkQueue						present_queue{ VK_NULL_HANDLE };
		VkQueue						transfer_queue{ VK_NULL_HANDLE };


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
			assert(instance);
			assert(!debug_messenger);
			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
			if (func != nullptr) 
			{
				return func(instance, create_info, allocator, &debug_messenger);
			}
			else 
			{
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		}

		void proxy_destroy_debug_utils_messenger_ext(const VkAllocationCallbacks* allocator) 
		{
			assert(instance);
			assert(debug_messenger);
			auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
			if (func != nullptr) 
			{
				func(instance, debug_messenger, allocator);
			}
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

		bool is_device_suitable(const VkPhysicalDevice& device,
			const std::vector<const char*>& device_extensions,
			bool check_queue_families = true)
		{
			vkGetPhysicalDeviceProperties(device, &device_properties);

			// checking support for optional features like texture compression, 
			// 64 bit floats and multi viewport rendering (useful for VR)
			vkGetPhysicalDeviceFeatures(device, &device_features);

			bool graphics_card_adequate{ device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
										 device_features.samplerAnisotropy &&
										 device_features.geometryShader };

			uint32_t queue_family_count = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

			std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

			bool found_queue_families{ false };
			int i{ 0 };
			for (const auto& family : queue_families)
			{
				if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
					queue_family_indices.graphics_family = i;

				if (family.queueFlags & VK_QUEUE_TRANSFER_BIT)
				{
					if (check_queue_families)
					{
						// look for a queue family with the VK_QUEUE_TRANSFER_BIT bit, but not the VK_QUEUE_GRAPHICS_BIT.
						if (!(family.queueFlags & VK_QUEUE_GRAPHICS_BIT))
							queue_family_indices.transfer_family = i;
					}
					else
					{
						queue_family_indices.transfer_family = i;
					}
				}

				VkBool32 present_support{ false };
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vk_surface.get_surface(), &present_support);
				if (present_support)
					queue_family_indices.present_family = i;

				if (check_queue_families)
				{
					found_queue_families = queue_family_indices.is_complete() &&
						(queue_family_indices.present_family.value() == queue_family_indices.graphics_family.value());
				}
				else
				{
					found_queue_families = queue_family_indices.is_complete();
				}

				if (found_queue_families)
					break;

				++i;
			}

			// existance of present family implies support for swap chain extension
			// however, for the sake of good practice, we explicitly check for this 
			// extension support here!
			bool all_extensions_supported{ vkh::check_device_extensions_support(device, device_extensions) };

			// there is at least one supported image format and one supported presentation mode given the window surface
			bool swap_chain_adequate{ false };
			if (all_extensions_supported)
			{
				vk_surface.populate_swap_chain_support_details(device);
				swap_chain_adequate = vk_surface.is_swap_chain_adequate();
			}

			return graphics_card_adequate && found_queue_families && all_extensions_supported && swap_chain_adequate;
		}

		void pick_physical_device(const std::vector<const char*>& device_extensions)
		{
			assert(instance);
			// get number of devices in system
			uint32_t device_count{ 0 };
			vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

			if (!device_count)
			{
				std::cout << "Found no vulkan suitable devices!\n";
				return;
			}

			assert(device_count);
			std::vector<VkPhysicalDevice> all_devices{ device_count };
			vkEnumeratePhysicalDevices(instance, &device_count, all_devices.data());

			// first explicitly prefer a physical device that supports drawing and presentation in the same queue for improved performance
			for (const auto& candidate : all_devices) 
			{
				if (is_device_suitable(candidate, device_extensions)) 
				{
					device = candidate;
					break;
				}
			}
			// if no device found relax the condition
			if (device == VK_NULL_HANDLE)
			{
				for (const auto& candidate : all_devices) 
				{
					if (is_device_suitable(candidate, device_extensions, false)) 
					{
						device = candidate;
						break;
					}
				}
			}

			if (device == VK_NULL_HANDLE) 
			{
				std::cout << "failed to find a suitable GPU!\n";
				return;
			}

		}

	}// anonymous namespace

	bool init(GLFWwindow* window)
	{
		VkApplicationInfo app_info{};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
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

		if (!vkh::check_validation_layers_support(validation_layers))
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
		std::vector<const char*> extensions{ vkh::get_required_extensions() };
		assert(extensions.data());

		create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		create_info.ppEnabledExtensionNames = extensions.data();

		// creating vulkan instance
		VKCALL(vkCreateInstance(&create_info, nullptr, &instance), "failed to create vulkan instance!");
		assert(instance);
		if (!instance) return false;

#if _DEBUG

		VKCALL(proxy_create_debug_utils_messenger_ext(&debug_create_info, nullptr), "failed to setup debug messenger!");
#endif

		// create surface
		if (!vk_surface.create_surface(window))
			return false;

		const std::vector<const char*> device_extensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		pick_physical_device(device_extensions);
		assert(device);
		if (!device)
			return false;

		// creating grpahics queue
		float queue_priority{ 1.f };
		assert(queue_family_indices.is_complete());

		std::vector<VkDeviceQueueCreateInfo> queue_create_infos{};
		std::set<uint32_t> unique_queue_families = { queue_family_indices.graphics_family.value(),
													queue_family_indices.present_family.value(),
													queue_family_indices.transfer_family.value()};

		for (uint32_t unique_family : unique_queue_families)
		{
			VkDeviceQueueCreateInfo queue_create_info{};
			queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info.queueFamilyIndex = unique_family;
			queue_create_info.queueCount = 1;
			queue_create_info.pQueuePriorities = &queue_priority;
			queue_create_infos.push_back(queue_create_info);
		}

		VkPhysicalDeviceFeatures enabled_device_features{};
		enabled_device_features.samplerAnisotropy = VK_TRUE;

		// Fill mode non solid is required for wireframe display
		if (device_features.fillModeNonSolid)
		{
			enabled_device_features.fillModeNonSolid = VK_TRUE;
		};

		// Wide lines must be present for line width > 1.0f
		if (device_features.wideLines)
		{
			enabled_device_features.wideLines = VK_TRUE;
		}

		// create logical device
		VkDeviceCreateInfo logical_device_create_info{};
		logical_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		logical_device_create_info.pQueueCreateInfos = queue_create_infos.data();
		logical_device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
		logical_device_create_info.pEnabledFeatures = &enabled_device_features;

		// specifying device specific extensions and validation layers
		logical_device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
		logical_device_create_info.ppEnabledExtensionNames = device_extensions.data();

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

		// retreive queue handles
		vkGetDeviceQueue(logical_device, queue_family_indices.graphics_family.value(), 0, &graphics_queue);
		vkGetDeviceQueue(logical_device, queue_family_indices.present_family.value(), 0, &present_queue);
		vkGetDeviceQueue(logical_device, queue_family_indices.transfer_family.value(), 0, &transfer_queue);

		memory::init();
		resources::init();
		// vk_descriptors.init();

		// TODO: remove?!
		// create swap chain
		if (!vk_surface.create_swap_chain(queue_family_indices.graphics_family.value(), queue_family_indices.present_family.value()))
			return false;
		// create command pool and buffers
		if (!vk_command.create_command_pool() || !vk_command.create_command_buffer())
			return false;

		return true;
	}

	void shutdown()
	{
#if _DEBUG
		proxy_destroy_debug_utils_messenger_ext(nullptr);
#endif
		vk_command.destroy();
		vk_surface.destroy();

		resources::shutdown();
		memory::shutdown();
		vkDestroyDevice(logical_device, nullptr);
		vkDestroyInstance(instance, nullptr);
	}

	VkInstance get_vulkan_instance() { return instance; }
	VkPhysicalDevice get_physical_device() { return device; }
	VkPhysicalDeviceProperties	get_physical_device_properties() { return device_properties; }
	VkDevice get_logical_device() { return logical_device; }

	VkQueue get_graphics_queue() { return graphics_queue; }
	uint32_t get_graphics_queue_family_index() { return queue_family_indices.graphics_family.value(); }
	VkQueue get_present_queue() { return present_queue; }
	uint32_t get_present_queue_family_index() { return queue_family_indices.present_family.value(); }
	VkQueue get_transfer_queue() { return transfer_queue; }
	uint32_t get_transfer_queue_family_index() { return queue_family_indices.transfer_family.value(); }

	uint32_t get_current_command_buffer_index()
	{
		return vk_command.get_current_command_buffer_index();
	}

	VkCommandBuffer get_command_buffer()
	{
		return vk_command.get_command_buffer();
	}

	VkFramebuffer get_frame_buffer()
	{
		return vk_surface.get_frame_buffer(vk_command.get_current_image_index());
	}

	VkExtent2D get_swap_chain_extent()
	{
		return vk_surface.get_swap_chain_extent();
	}

	float get_extent_aspect_ratio()
	{
		return vk_surface.get_extent_aspect_ratio();
	}

	VkFormat get_swap_chain_image_format()
	{
		return vk_surface.get_image_format();
	}

	VkFormat get_swap_chain_depth_format()
	{
		return vk_surface.get_depth_format();
	}

	bool create_frame_buffers(VkRenderPass render_pass)
	{
		return vk_surface.create_frame_buffers(render_pass);
	}

	void begin_frame()
	{
		vk_command.begin_frame(&vk_surface);
	}

	void end_frame()
	{
		vk_command.end_frame(&vk_surface, graphics_queue, present_queue);
	}

	void frame_buffer_resize_callback(GLFWwindow* window, int width, int height)
	{
		vk_command.set_frame_buffer_resized();
	}

	void transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout)
	{
		vk_command.transition_image_layout(image, format, old_layout, new_layout, graphics_queue);
	}

}