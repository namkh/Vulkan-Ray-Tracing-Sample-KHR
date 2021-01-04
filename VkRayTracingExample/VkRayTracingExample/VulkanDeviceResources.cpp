
#include "VulkanDeviceResources.h"

#include <vulkan/vulkan_win32.h>
#include <math.h>

#include "CommandBuffers.h"

bool VulkanDeviceResources::Initialize(HINSTANCE hAppInstance, HWND hMainWnd, uint32_t width, uint32_t height, bool useRayTracing)
{
	m_win32Instance = hAppInstance;
	m_win32Wnd		= hMainWnd;
	m_width			= width;
	m_height		= height;
	m_useRayTracing = useRayTracing;
	
	if (!InitDevice())
	{
		return false;
	}
	if (!InitCommandResources())
	{
		return false;
	}
	if (!InitDeviceQueue())
	{
		return false;
	}
	if (!InitSwapChain())
	{
		return false;
	}
	if (!InitSwapChainResources())
	{
		return false;
	}

	m_screenSizeChangedEventHandle = gCoreEventMgr.RegisterScreenSizeChangedEventCallback(this, &VulkanDeviceResources::OnScreenSizeChanged);

	return true;
}

void VulkanDeviceResources::Destroy()
{
	OnRenderTargetSizeChanged.Clear();

	gCoreEventMgr.UnregisterEventCallback(m_screenSizeChangedEventHandle);

	WaitForAllDeviceAction();

	DestroySwapChain();

	if (m_defaultCommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(m_logicalDevice, m_defaultCommandPool, nullptr);
	}

	if (m_surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(m_vkInstance, m_surface, nullptr);
	}

	if (m_logicalDevice != VK_NULL_HANDLE)
	{
		vkDestroyDevice(m_logicalDevice, nullptr);
	}
	if (m_vkInstance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(m_vkInstance, nullptr);
	}
}

void VulkanDeviceResources::DestroySwapChain()
{
	if (m_depthBuffer.m_imageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(m_logicalDevice, m_depthBuffer.m_imageView, nullptr);
	}

	if (m_depthBuffer.m_deviceMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(m_logicalDevice, m_depthBuffer.m_deviceMemory, nullptr);
	}

	if (m_depthBuffer.m_image != VK_NULL_HANDLE)
	{
		vkDestroyImage(m_logicalDevice, m_depthBuffer.m_image, nullptr);
	}
	
	for (uint32_t i = 0; i < m_swapchainBuffers.size(); i++)
	{
		if (m_swapchainBuffers[i].m_imageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(m_logicalDevice, m_swapchainBuffers[i].m_imageView, nullptr);
		}
	}
	m_swapchainBuffers.clear();

	if (m_swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(m_logicalDevice, m_swapchain, nullptr);
	}
}

bool VulkanDeviceResources::InitDevice()
{
	volkInitialize();
#if _DEBUG
	uint32_t numInstanceLayerProperties = 0;
	vkEnumerateInstanceLayerProperties(&numInstanceLayerProperties, nullptr);
	if (numInstanceLayerProperties < 1)
	{
		REPORT(EReportType::REPORT_TYPE_WARN, "Debug layer activation failed.");
	}
	else
	{
		m_layerProperties.resize(numInstanceLayerProperties);
		vkEnumerateInstanceLayerProperties(&numInstanceLayerProperties, m_layerProperties.data());
	}
	std::vector<const char*> instanceDebugLayerNames;
	for (const auto& cur : m_layerProperties)
	{
		if (strcmp(cur.layerName, "VK_LAYER_KHRONOS_validation") == 0)
		{
			instanceDebugLayerNames.push_back("VK_LAYER_KHRONOS_validation");
		}
	}
#endif

	std::vector<const char*> extensionNames;
	uint32_t numInstanceExtensions = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &numInstanceExtensions, nullptr);
	if (numInstanceExtensions > 0)
	{
		std::vector<VkExtensionProperties> extensionProperties;
		extensionProperties.resize(numInstanceExtensions);
		vkEnumerateInstanceExtensionProperties(nullptr, &numInstanceExtensions, extensionProperties.data());
		for (uint32_t i = 0; i < numInstanceExtensions; i++)
		{
			if (strcmp(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, extensionProperties[i].extensionName) == 0)
			{
				extensionNames.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
			}
#if _DEBUG
			/*if (strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, extensionProperties[i].extensionName) == 0)
			{
				extensionNames.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
			}*/
#endif
			if (strcmp(VK_KHR_SURFACE_EXTENSION_NAME, extensionProperties[i].extensionName) == 0)
			{
				extensionNames.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
			}
			if (strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, extensionProperties[i].extensionName) == 0)
			{
				extensionNames.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
			}
		}
	}
	else
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "No extension found.");
		return false;
	}

	/*create vulkan instance*/
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = "HelloVulkan";
	appInfo.applicationVersion = 1;
	appInfo.pEngineName = "HelloVulkanEngine";
	appInfo.engineVersion = 1;
	appInfo.apiVersion = VK_API_VERSION_1_2;

	VkInstanceCreateInfo instanceCretateInfo = {};
	instanceCretateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCretateInfo.pNext = nullptr;
	instanceCretateInfo.flags = 0;
	instanceCretateInfo.pApplicationInfo = &appInfo;
	instanceCretateInfo.enabledExtensionCount = static_cast<uint32_t>(extensionNames.size());
	instanceCretateInfo.ppEnabledExtensionNames = extensionNames.data();

	VkResult res = vkCreateInstance(&instanceCretateInfo, nullptr, &m_vkInstance);
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Vulkan instance create failed.");
		return false;
	}

	volkLoadInstance(m_vkInstance);

	/*get physical devices*/
	uint32_t numPhysicalDevices;
	vkEnumeratePhysicalDevices(m_vkInstance, &numPhysicalDevices, nullptr);
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "There are no physical devices.");
		return false;
	}
	m_physicalDevices.resize(numPhysicalDevices); 
	res = vkEnumeratePhysicalDevices(m_vkInstance, &numPhysicalDevices, m_physicalDevices.data());
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Failed to acquire physical device.");
		return false;
	}

	/* get device properties */
	if (m_useRayTracing)
	{
		m_physicalDeviceRayTracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_KHR;
		VkPhysicalDeviceProperties2 physicalDeviceProp2 = {};
		physicalDeviceProp2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		physicalDeviceProp2.pNext = &m_physicalDeviceRayTracingProperties;
		vkGetPhysicalDeviceProperties2(m_physicalDevices[0], &physicalDeviceProp2);
		m_physicalDeviceProperty = physicalDeviceProp2.properties;

		m_physicalDeviceRayTracingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_FEATURES_KHR;
		VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
		physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		physicalDeviceFeatures2.pNext = &m_physicalDeviceRayTracingFeatures;
		vkGetPhysicalDeviceFeatures2(m_physicalDevices[0], &physicalDeviceFeatures2);
		m_physicalDeviceFeatures = physicalDeviceFeatures2.features;
	}
	else
	{
		vkGetPhysicalDeviceProperties(m_physicalDevices[0], &m_physicalDeviceProperty);
		vkGetPhysicalDeviceFeatures(m_physicalDevices[0], &m_physicalDeviceFeatures);
	}

	VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeature = {};
	descriptorIndexingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
	physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	physicalDeviceFeatures2.pNext = &descriptorIndexingFeature;
	vkGetPhysicalDeviceFeatures2(m_physicalDevices[0], &physicalDeviceFeatures2);

	VkPhysicalDeviceBufferDeviceAddressFeaturesKHR physicalDeviceBufferDeviceAddr = {};
	physicalDeviceBufferDeviceAddr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR;
	physicalDeviceFeatures2 = {};
	physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	physicalDeviceFeatures2.pNext = &physicalDeviceBufferDeviceAddr;
	vkGetPhysicalDeviceFeatures2(m_physicalDevices[0], &physicalDeviceFeatures2);

	vkGetPhysicalDeviceMemoryProperties(m_physicalDevices[0], &m_physicalDeviceMemoryProperty);

	uint32_t numQueueFamilys = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevices[0], &numQueueFamilys, nullptr);
	if (numQueueFamilys < 1)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "The number of queue families is zero.");
		return false;
	}

	m_queueFamilyProperties.resize(numQueueFamilys);
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevices[0], &numQueueFamilys, m_queueFamilyProperties.data());

	VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo = {};
	win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	win32SurfaceCreateInfo.pNext = nullptr;
	win32SurfaceCreateInfo.flags = 0;
	win32SurfaceCreateInfo.hinstance = m_win32Instance;
	win32SurfaceCreateInfo.hwnd = m_win32Wnd;

	res = vkCreateWin32SurfaceKHR(m_vkInstance, &win32SurfaceCreateInfo, nullptr, &m_surface);
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Win32 surface create failed.");
		return false;
	}

	VkBool32* supportPresent = new VkBool32[numQueueFamilys];
	for (uint32_t i = 0; i < numQueueFamilys; i++)
	{
		vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevices[0], i, m_surface, &supportPresent[i]);
	}

	/* create logical device  */
	bool queueFinded = false;
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	for (uint32_t i = 0; i < m_queueFamilyProperties.size(); i++)
	{
		if (m_queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			if (vkGetPhysicalDeviceWin32PresentationSupportKHR(m_physicalDevices[0], i) && supportPresent[i])
			{
				queueCreateInfo.queueFamilyIndex = i;
				m_graphicsQueueFamilyIndex = i;
				m_presentQueueFamilyIndex = i;
				queueFinded = true;
				break;
			}
		}
	}

	delete [] supportPresent;

	if (!queueFinded)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Queue family index search failed.");
		return false;
	}

	float queue_priorities[1] = { 0.0f };
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.pNext = nullptr;
	queueCreateInfo.flags = 0;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = queue_priorities;

	extensionNames.clear();
	uint32_t numDeviceExtensionProps = 0;
	res = vkEnumerateDeviceExtensionProperties(m_physicalDevices[0], nullptr, &numDeviceExtensionProps, nullptr);
	if (res == VkResult::VK_SUCCESS)
	{
		std::vector<VkExtensionProperties> extensionProperties;
		extensionProperties.resize(numDeviceExtensionProps);
		res = vkEnumerateDeviceExtensionProperties(m_physicalDevices[0], nullptr, &numDeviceExtensionProps, extensionProperties.data());
		for (uint32_t i = 0; i < numDeviceExtensionProps; i++)
		{	
			if (strcmp(VK_KHR_SURFACE_EXTENSION_NAME, extensionProperties[i].extensionName) == 0)
			{
				extensionNames.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
			}
			if (strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, extensionProperties[i].extensionName) == 0)
			{
				extensionNames.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
			}
			if (strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, extensionProperties[i].extensionName) == 0)
			{
				extensionNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
			}
			if (strcmp(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME, extensionProperties[i].extensionName) == 0)
			{
				extensionNames.push_back(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
			}
			if (m_useRayTracing)
			{
				if (strcmp(VK_KHR_RAY_TRACING_EXTENSION_NAME, extensionProperties[i].extensionName) == 0)
				{
					extensionNames.push_back(VK_KHR_RAY_TRACING_EXTENSION_NAME);
				}
				if (strcmp(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME, extensionProperties[i].extensionName) == 0)
				{
					extensionNames.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
				}
				if (strcmp(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, extensionProperties[i].extensionName) == 0)
				{
					extensionNames.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
				}
				if (strcmp(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, extensionProperties[i].extensionName) == 0)
				{
					extensionNames.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
				}
				if (strcmp(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, extensionProperties[i].extensionName) == 0)
				{
					extensionNames.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
				}
				if (strcmp(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME, extensionProperties[i].extensionName) == 0)
				{
					extensionNames.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
				}
				if (strcmp(VK_KHR_MAINTENANCE3_EXTENSION_NAME, extensionProperties[i].extensionName) == 0)
				{
					extensionNames.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
				}
			}
		}
	}

	m_physicalDeviceRayTracingFeatures.pNext = &physicalDeviceBufferDeviceAddr;
	physicalDeviceBufferDeviceAddr.pNext = &descriptorIndexingFeature;
	VkDeviceCreateInfo logicalDeviceCreateInfo = {};
	logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	logicalDeviceCreateInfo.pNext = &m_physicalDeviceRayTracingFeatures;
	logicalDeviceCreateInfo.flags = 0;
	logicalDeviceCreateInfo.queueCreateInfoCount = 1;
	logicalDeviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	logicalDeviceCreateInfo.enabledLayerCount = 0;
	logicalDeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensionNames.size());
#if _DEBUG
	logicalDeviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(instanceDebugLayerNames.size());
	logicalDeviceCreateInfo.ppEnabledLayerNames = instanceDebugLayerNames.data();
#else
	logicalDeviceCreateInfo.enabledLayerCount = 0;
	logicalDeviceCreateInfo.ppEnabledLayerNames = nullptr;
#endif
	logicalDeviceCreateInfo.ppEnabledExtensionNames = extensionNames.data();
	logicalDeviceCreateInfo.pEnabledFeatures = &m_physicalDeviceFeatures;

	res = vkCreateDevice(m_physicalDevices[0], &logicalDeviceCreateInfo, nullptr, &m_logicalDevice);
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Logical deive create failed.");
		return false;
	}

	volkLoadDevice(m_logicalDevice);

	return true;
}

bool VulkanDeviceResources::InitCommandResources()
{
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.pNext = nullptr;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = m_graphicsQueueFamilyIndex;

	VkResult res = vkCreateCommandPool(m_logicalDevice, &commandPoolCreateInfo, nullptr, &m_defaultCommandPool);
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Command pool create failed.");
		return false;
	}

	return true;
}

bool VulkanDeviceResources::InitDeviceQueue()
{
	vkGetDeviceQueue(m_logicalDevice, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);

	if (m_graphicsQueueFamilyIndex == m_presentQueueFamilyIndex)
	{
		m_presentQueue = m_graphicsQueue;
	}
	else
	{
		vkGetDeviceQueue(m_logicalDevice, m_presentQueueFamilyIndex, 0, &m_presentQueue);
	}

	return true;
}

bool VulkanDeviceResources::InitSwapChain()
{
	uint32_t formatCount = 0;
	VkResult res = vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevices[0], m_surface, &formatCount, nullptr);
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "There is no surface format.");
		return false;
	}
	std::vector<VkSurfaceFormatKHR> surfaceFormat;
	surfaceFormat.resize(formatCount);
	res = vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevices[0], m_surface, &formatCount, surfaceFormat.data());
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Can't get surface format.");
		return false;
	}

	if (formatCount >= 1)
	{
		if (formatCount == 1 && surfaceFormat[0].format == VK_FORMAT_UNDEFINED)
		{
			m_backbufferFormat = VK_FORMAT_B8G8R8A8_UNORM;
		}
		else
		{
			m_backbufferFormat = surfaceFormat[0].format;
		}
	}
	else
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "The surface type could not be found.");
		return false;
	}

	VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
	res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevices[0], m_surface, &surfaceCapabilities);
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Can't get VkSurfaceCapabilitiesKHR.");
		return false;
	}

	uint32_t presentModeCount = 0;
	res = vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevices[0], m_surface, &presentModeCount, nullptr);
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "There is no present mode.");
		return false;
	}
	std::vector<VkPresentModeKHR> presentModes;
	presentModes.resize(presentModeCount);
	res = vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevices[0], m_surface, &presentModeCount, presentModes.data());
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Can't get present mode");
		return false;
	}

	if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF)
	{
		m_swapchainExtent.width	= m_width;
		m_swapchainExtent.height = m_height;
		
		m_swapchainExtent.width = std::max(m_swapchainExtent.width, surfaceCapabilities.minImageExtent.width);
		m_swapchainExtent.height = std::max(m_swapchainExtent.height, surfaceCapabilities.minImageExtent.height);

		m_swapchainExtent.width = std::min(m_swapchainExtent.width, surfaceCapabilities.maxImageExtent.width);
		m_swapchainExtent.height = std::min(m_swapchainExtent.height, surfaceCapabilities.maxImageExtent.height);
	}
	else
	{
		m_swapchainExtent = surfaceCapabilities.currentExtent;
	}

	VkPresentModeKHR swapchainPresentMode = presentModes[0];
	uint32_t desiredNumberOfSwapChainImages = surfaceCapabilities.minImageCount;

	VkSurfaceTransformFlagBitsKHR preTransform;
	if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		preTransform = surfaceCapabilities.currentTransform;
	}

	VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags =
	{
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
	};

	for (uint32_t i = 0; i < compositeAlphaFlags.size(); i++)
	{
		if (surfaceCapabilities.supportedCompositeAlpha & compositeAlphaFlags[i])
		{
			compositeAlpha = compositeAlphaFlags[i];
			break;
		}
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.pNext = nullptr;
	swapchainCreateInfo.flags = 0;
	swapchainCreateInfo.surface = m_surface;
	swapchainCreateInfo.minImageCount = desiredNumberOfSwapChainImages;
	swapchainCreateInfo.imageFormat = m_backbufferFormat;
	swapchainCreateInfo.imageColorSpace = surfaceFormat[0].colorSpace;
	swapchainCreateInfo.imageExtent = m_swapchainExtent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	swapchainCreateInfo.preTransform = preTransform;
	swapchainCreateInfo.compositeAlpha = compositeAlpha;
	swapchainCreateInfo.presentMode = swapchainPresentMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
	if (m_graphicsQueueFamilyIndex != m_presentQueueFamilyIndex)
	{
		uint32_t queueFamilyIndices[2] =
		{
			static_cast<uint32_t>(m_graphicsQueueFamilyIndex),
			static_cast<uint32_t>(m_presentQueueFamilyIndex)
		};

		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	res = vkCreateSwapchainKHR(m_logicalDevice, &swapchainCreateInfo, nullptr, &m_swapchain);
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Swap chain create failed.");
		return false;
	}

	return true;
}

bool VulkanDeviceResources::InitSwapChainResources()
{
	VkResult res = vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &m_swapchainBufferCount, nullptr);
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "The number of swap chains is zero.");
		return false;
	}
	std::vector<VkImage> swapchainImages;
	swapchainImages.resize(m_swapchainBufferCount);
	res = vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &m_swapchainBufferCount, swapchainImages.data());
	m_swapchainBuffers.resize(m_swapchainBufferCount);
	for (uint32_t i = 0; i < swapchainImages.size(); i++)
	{
		m_swapchainBuffers[i].m_image = swapchainImages[i];
	}

	bool swapchainImageViewCreated = false;
	for (uint32_t i = 0; i < m_swapchainBuffers.size(); i++)
	{
		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = m_swapchainBuffers[i].m_image;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = m_backbufferFormat;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		res = vkCreateImageView(m_logicalDevice, &imageViewCreateInfo, nullptr, &m_swapchainBuffers[i].m_imageView);
		if (res != VkResult::VK_SUCCESS)
		{
			//스왑체인 이미지 뷰 생성실패 로깅
			return false;
		}
	}

	VkImageSubresourceRange subResourceRange = {};
	subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResourceRange.baseMipLevel = 0;
	subResourceRange.levelCount = 1;
	subResourceRange.baseArrayLayer = 0;
	subResourceRange.layerCount = 1;

	SingleTimeCommandBuffer singleTimeCommandBuffer;
	singleTimeCommandBuffer.Begin();
	VkImageMemoryBarrier backBufferBarrier{};
	backBufferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	backBufferBarrier.srcAccessMask = 0;
	backBufferBarrier.dstAccessMask = 0;
	backBufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	backBufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	backBufferBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	backBufferBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	backBufferBarrier.subresourceRange = subResourceRange;
	for (int i = 0; i < m_swapchainBuffers.size(); i++)
	{
		backBufferBarrier.image = m_swapchainBuffers[i].m_image;
		vkCmdPipelineBarrier
		(
			singleTimeCommandBuffer.GetCommandBuffer(),
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&backBufferBarrier
		);
	}
	singleTimeCommandBuffer.End();

	VkImageCreateInfo imageCreateInfo = {};
	VkFormatProperties deviceFormatProps = {};
	vkGetPhysicalDeviceFormatProperties(m_physicalDevices[0], m_depthBufferFormat, &deviceFormatProps);
	if (deviceFormatProps.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
	}
	else if (deviceFormatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	}
	else
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Depth stencil format is not supported.");
		return false;
	}

	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = nullptr;
	imageCreateInfo.flags = 0;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = m_depthBufferFormat;
	imageCreateInfo.extent.width = m_swapchainExtent.width;
	imageCreateInfo.extent.height = m_swapchainExtent.height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = NUM_SAMPLES;
	imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	//차후 렌더큐 여러개 쓰개되면 맞춰줘야함
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = nullptr;

	VkMemoryAllocateInfo memAllocInfo = {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.pNext = nullptr;
	memAllocInfo.allocationSize = 0;
	memAllocInfo.memoryTypeIndex = 0;

	VkImageViewCreateInfo imageViewCreateInfo = {};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.pNext = nullptr;
	imageViewCreateInfo.flags = 0;
	imageViewCreateInfo.image = VK_NULL_HANDLE;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = m_depthBufferFormat;
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;

	res = vkCreateImage(m_logicalDevice, &imageCreateInfo, nullptr, &m_depthBuffer.m_image);
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Depth stencil image create failed.");
		return false;
	}

	VkMemoryRequirements memRequirements = {};
	vkGetImageMemoryRequirements(m_logicalDevice, m_depthBuffer.m_image, &memRequirements);
	memAllocInfo.allocationSize = memRequirements.size;
	if (MemoryTypeFromProperties(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllocInfo.memoryTypeIndex))
	{
		res = vkAllocateMemory(m_logicalDevice, &memAllocInfo, nullptr, &m_depthBuffer.m_deviceMemory);
		if (res != VkResult::VK_SUCCESS)
		{
			REPORT(EReportType::REPORT_TYPE_ERROR, "Depth stencil memory create failed.");
			return false;
		}
		res = vkBindImageMemory(m_logicalDevice, m_depthBuffer.m_image, m_depthBuffer.m_deviceMemory, 0);
		if (res != VkResult::VK_SUCCESS)
		{
			REPORT(EReportType::REPORT_TYPE_ERROR, "Depth stencil image bind failed.");
			return false;
		}
		imageViewCreateInfo.image = m_depthBuffer.m_image;
		res = vkCreateImageView(m_logicalDevice, &imageViewCreateInfo, nullptr, &m_depthBuffer.m_imageView);
		if (res != VkResult::VK_SUCCESS)
		{
			REPORT(EReportType::REPORT_TYPE_ERROR, "Depth stencil image view create failed.");
			return false;
		}
	}

	return true;
}

void VulkanDeviceResources::RefreshSwapchain()
{
	WaitForAllDeviceAction();

	DestroySwapChain();

	InitSwapChain();
	InitSwapChainResources();

	if (OnRenderTargetSizeChanged.GetCommandCount() > 0)
	{
		OnRenderTargetSizeChanged.Exec();
	}

	WaitForAllDeviceAction();

	m_isSwapchainDirty = false;
}

void VulkanDeviceResources::GraphicsQueueWaitIdle()
{
	vkQueueWaitIdle(m_graphicsQueue);
}

void VulkanDeviceResources::WaitForAllDeviceAction()
{
	vkQueueWaitIdle(m_graphicsQueue);
	vkQueueWaitIdle(m_presentQueue);
	vkDeviceWaitIdle(m_logicalDevice);
}

void VulkanDeviceResources::RestoreDeviceIfDirtied()
{
	if (m_isSwapchainDirty)
	{
		RefreshSwapchain();
	}
}

void VulkanDeviceResources::OnScreenSizeChanged(ScreenSizeChangedEvent* screenSizeChangedEvent)
{
	m_width = static_cast<uint32_t>(screenSizeChangedEvent->Width);
	m_height = static_cast<uint32_t>(screenSizeChangedEvent->Height);
	m_isSwapchainDirty = true;
}

bool VulkanDeviceResources::MemoryTypeFromProperties(int32_t typeBits, VkFlags requirementsMask, uint32_t* typeIndex)
{
	for (uint32_t i = 0; i < m_physicalDeviceMemoryProperty.memoryTypeCount; i++)
	{
		if ((typeBits & (1 << i)))
		{
			if ((m_physicalDeviceMemoryProperty.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask)
			{
				*typeIndex = i;
				return true;
			}
		}
	}
	return false;
}