#pragma once

#include "SimpleRenderObject.h"

#include "RTAccelerationStructure.h"
#include "RTPipeline.h"
#include "RTPipelineResources.h"
#include "RTShaderBindingTable.h"

class RayTracer
{
public:
	bool Initialize(uint32_t width, uint32_t height, VkFormat rtTargetFormat);
	void Update(GlobalConstants& globalConstants, uint32_t frameIndex);
	void Destroy();

public:

	void LoadRayGenShader(std::string& rgenFilePath);
	void LoadMissShader(std::string& missFilePath);
	void SetEnvCubemap(SimpleCubmapTexture* envCubmapTexture) { m_envCubmapTexture = envCubmapTexture; }

	bool Build();

public:
	std::vector<CommandBuffer*>& GetWaitCommandBuffer() { return m_currentCommandBuffers; }

protected:
	void RebuildCommandBuffer();
	bool BuildCommandBuffers();
	

public:
	void OnScreenSizeChanged(uint32_t width, uint32_t height);

private:

	VkCommandPool m_commandPool = VK_NULL_HANDLE;

	RtTargetImageBuffer m_rtTargetImage;
	SimpleCubmapTexture* m_envCubmapTexture;

	RTPipelineResources m_pipelineResources = {};
	RTPipeline m_pipeline = {};
	RTShaderBindingTable m_shaderBindingTable = {};
	RTAccelerationStructure m_accelerationStructure = {};
	StaticCommandBufferContainer m_commandBufferContainer = {};

	std::vector<CommandBuffer*> m_currentCommandBuffers = {};
	
	std::vector<SimpleShader*> m_rayGenShaders;
	std::vector<SimpleShader*> m_missShaders;

	uint32_t m_width = 0;
	uint32_t m_height = 0;

	CommandHandle m_screenSizeChangedEventHandle = INVALID_COMMAND_HANDLE;
};

