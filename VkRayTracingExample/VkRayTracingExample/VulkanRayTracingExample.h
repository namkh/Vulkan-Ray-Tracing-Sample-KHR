#pragma once

#include "VulkanDeviceResources.h"
#include "ExampleAppBase.h"
#include "SphericalCoordinateMovementCamera.h"

#include "RenderObjectContainer.h"
#include "RayTracer.h"
#include "Fence.h"

class VulkanRayTracingExample : public ExampleAppBase
{
public:
	VulkanRayTracingExample(std::wstring appName, int width, int height, bool useRayTracing)
		: ExampleAppBase(appName, width, height, useRayTracing)
	{
	}

public:
	virtual bool Initialize() override;
	virtual void Update(float timeDelta) override;
	virtual void Render() override;
	virtual void Destroy() override;

public:
	void UpdateTransformAnimation();

public:
	void OnScreenSizeChanged();

private:

	SphericalCoordinateMovementCamera m_camera;
	SimpleCubmapTexture m_envCubmapTexture;

	std::vector<VkSemaphore> m_imageAcquiredSemaphore = {};
	std::vector<VkSemaphore> m_renderCompleteSemaphore = {};
	std::vector<Fence>		 m_drawFence = {};
	std::vector<VkFence>	 m_imageFence = {};

	VkPipelineStageFlags m_submitPipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	uint32_t m_currentFrame = 0;

	RayTracer m_rayTracer = {};

	GlobalConstants m_globalConstants = {};

	std::vector<SimpleRenderObject*> m_renderObjects = {};
	std::vector<SampleRenderObjectInstance*> m_roteteInstances = {};

	uint32_t m_currentBuffer = 0;

	static const uint32_t NUM_FRAMES = 2;
};

