#pragma once

#include <wrl.h>
#include <Combaseapi.h>
#include <functional>
#include <vector>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include "volk.h"

#include "CoreEventManager.h"
#include "Singleton.h"
#include "Utils.h"

struct SwapchainBuffer
{
	VkImage		m_image = VK_NULL_HANDLE;
	VkImageView m_imageView = VK_NULL_HANDLE;
};

struct DepthBuffer
{
	VkImage			m_image = VK_NULL_HANDLE;
	VkImageView		m_imageView = VK_NULL_HANDLE;
	VkDeviceMemory	m_deviceMemory = VK_NULL_HANDLE;
};

class VulkanDeviceResources : public TSingleton<VulkanDeviceResources>
{
public:
	VulkanDeviceResources(token) {};

	bool Initialize(HINSTANCE hAppInstance, HWND hMainWnd, uint32_t width, uint32_t height, bool useRayTracing = false);
	void Destroy();
	void DestroySwapChain();

public:
	bool InitDevice();
	bool InitCommandResources();
	bool InitDeviceQueue();
	bool InitSwapChain();
	bool InitSwapChainResources();

	void RefreshSwapchain();

public:

	void GraphicsQueueWaitIdle();
	void WaitForAllDeviceAction();
	void RestoreDeviceIfDirtied();

public:
	void OnScreenSizeChanged(ScreenSizeChangedEvent* screenSizeChangedEvent);

	LambdaCommandList<std::function<void()> > OnRenderTargetSizeChanged;
public:

	VkInstance&			GetVkInstance()				{ return m_vkInstance; }
	VkDevice&			GetLogicalDevice()			{ return m_logicalDevice;}
	VkSurfaceKHR&		GetSurface()				{ return m_surface; }
	VkSwapchainKHR&		GetSwapchain()				{ return m_swapchain; }
	VkQueue&			GetGraphicsQueue()			{ return m_graphicsQueue; }
	VkQueue&			GetPresentQueue()			{ return m_presentQueue; }
	VkCommandPool&		GetDefaultCommandPool()		{ return m_defaultCommandPool; }

	uint32_t GetGraphicsQueueFamilyIndex() { return m_graphicsQueueFamilyIndex; }
	uint32_t GetPresentQueueFamilyIndex() { return m_presentQueueFamilyIndex; }
	uint32_t GetComputeQueueFamilyIndex() { return m_computeQueueFamilyIndex; };

	

	VkPhysicalDeviceProperties&					GetPhysicalDeviceProperty()				{ return m_physicalDeviceProperty; }
	VkPhysicalDeviceFeatures&					GetPhysicalDeviceFeatures()				{ return m_physicalDeviceFeatures; }
	VkPhysicalDeviceMemoryProperties&			GetPhysicalDeviceMemoryProperty()		{ return m_physicalDeviceMemoryProperty; }
	
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR&	GetPhysicalDeviceRayTracingPipelineProperties()		{ return m_physicalDeviceRayTracingPipelineProperties; }
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR&		GetPhysicalDeviceRayTracingPipelineFeatures()		{ return m_physicalDeviceRayTracingPipelineFeatures; }
	VkPhysicalDeviceAccelerationStructurePropertiesKHR&	GetPhysicalDeviceAccelerationStructureProperties()	{ return m_physicalDeviceAccelerationStructureProperties; }
	VkPhysicalDeviceAccelerationStructureFeaturesKHR&	GetPhysicalDeviceAccelerationStructureFeatures()	{ return m_physicalDeviceAccelerationStructureFeatures; }

	DepthBuffer* GetDepthBuffer() { return &m_depthBuffer; }
	SwapchainBuffer* GetSwapChainBuffer(int index) 
	{
		if (index >= 0 && index < m_swapchainBuffers.size())
		{
			return &m_swapchainBuffers[index];
		}
		return nullptr;
	}

	uint32_t GetSwapchainBufferCount() { return m_swapchainBufferCount; }

	VkFormat GetBackbufferFormat() { return m_backbufferFormat; }
	VkFormat GetDepthBufferFormat() { return m_depthBufferFormat; }

	uint32_t GetWidth() { return m_width; }
	uint32_t GetHeight() { return m_height; }

public:

	bool MemoryTypeFromProperties(int32_t typeBits, VkFlags requirementsMask, uint32_t* typeIndex);

protected:

	VkInstance						m_vkInstance			= VK_NULL_HANDLE;
	VkDevice						m_logicalDevice			= VK_NULL_HANDLE;
	VkCommandPool					m_defaultCommandPool	= VK_NULL_HANDLE;
	VkQueue							m_graphicsQueue			= VK_NULL_HANDLE;
	VkQueue							m_presentQueue			= VK_NULL_HANDLE;
	VkSurfaceKHR					m_surface				= VK_NULL_HANDLE;
	VkSwapchainKHR					m_swapchain				= VK_NULL_HANDLE;

	std::vector<VkPhysicalDevice> m_physicalDevices;
	
	std::vector<SwapchainBuffer> m_swapchainBuffers;
	VkExtent2D					 m_swapchainExtent;
	DepthBuffer					 m_depthBuffer;
	
	VkPhysicalDeviceProperties						m_physicalDeviceProperty = {};
	VkPhysicalDeviceFeatures						m_physicalDeviceFeatures = {};
	VkPhysicalDeviceMemoryProperties				m_physicalDeviceMemoryProperty = {};

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR		m_physicalDeviceRayTracingPipelineProperties = {};
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR		m_physicalDeviceRayTracingPipelineFeatures = {};
	VkPhysicalDeviceAccelerationStructurePropertiesKHR	m_physicalDeviceAccelerationStructureProperties = {};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR	m_physicalDeviceAccelerationStructureFeatures = {};

	std::vector<VkLayerProperties>			m_layerProperties;
	std::vector<VkQueueFamilyProperties>	m_queueFamilyProperties;

	VkFormat m_backbufferFormat	= VK_FORMAT_UNDEFINED;
	VkFormat m_depthBufferFormat = VK_FORMAT_D16_UNORM;

	uint32_t m_graphicsQueueFamilyIndex = 0;
	uint32_t m_presentQueueFamilyIndex = 0;
	uint32_t m_computeQueueFamilyIndex = 0;

	HINSTANCE	m_win32Instance	= nullptr;
	HWND		m_win32Wnd		= nullptr;
	uint32_t	m_width			= 0;
	uint32_t	m_height		= 0;

	uint32_t m_swapchainBufferCount = 0;

	bool m_useRayTracing	= false;
	bool m_isSwapchainDirty = false;

	CoreEventHandle m_screenSizeChangedEventHandle = {};
};

#define gVkDeviceRes VulkanDeviceResources::Instance()
#define gLogicalDevice VulkanDeviceResources::Instance().GetLogicalDevice()
#define gVkInstance VulkanDeviceResources::Instance().GetVkInstance()