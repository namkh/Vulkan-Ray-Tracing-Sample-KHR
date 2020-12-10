#pragma once

#include "ShaderContainer.h"
#include "SimpleRenderObject.h"
#include "RTAccelerationStructure.h"
#include "DeviceBuffers.h"

class RTPipeline
{
public:
	bool Build(VkPipelineLayout pipelineLayout);
	void Destroy();

	VkPipeline GetPipeline() { return m_pipeline; }

public:
	uint32_t GetRayGenShaderGroupCount() { return m_rayGenShaderGroupCount; }
	uint32_t GetMissShaderGroupCount() { return m_missShaderGroupCount; }
	uint32_t GetHitShaderGroupCount() { return m_hitShaderGroupCount; }
	uint32_t GetCallableShaderGroupCount() { return m_callableShaderGroupCount; }
	uint32_t GetShaderGroupCount() { return m_shaderGroupCount; }

protected:
	void SetShaderStageCreateInfo(ERTShaderGroupType groupType, std::vector<VkPipelineShaderStageCreateInfo>& creatInfoBuffer, uint32_t& stageCreateInfoIndex);

private:
	VkPipeline m_pipeline = VK_NULL_HANDLE;

	uint32_t m_rayGenShaderGroupCount = 0;
	uint32_t m_missShaderGroupCount = 0;
	uint32_t m_hitShaderGroupCount = 0;
	uint32_t m_callableShaderGroupCount = 0;
	uint32_t m_shaderGroupCount = 0;

	std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_shaderGroupCreateInfos;
	static const VkShaderStageFlagBits VK_RAY_TRACING_SHADER_TYPE[SHADER_TYPE_END];
};

