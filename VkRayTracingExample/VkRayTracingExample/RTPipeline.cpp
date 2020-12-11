#include "RTPipeline.h"

#include "ShaderContainer.h"

const VkShaderStageFlagBits RTPipeline::VK_RAY_TRACING_SHADER_TYPE[SHADER_TYPE_END] =
{
	VK_SHADER_STAGE_RAYGEN_BIT_KHR,
	VK_SHADER_STAGE_MISS_BIT_KHR,
	VK_SHADER_STAGE_CALLABLE_BIT_KHR,
	VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
	VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
	VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
};

bool RTPipeline::Build(VkPipelineLayout pipelineLayout)
{
	if (m_pipeline != VK_NULL_HANDLE)
	{
		Destroy();
	}

	uint32_t allShaderCount = gShaderContainer.GetShaderCount();
	std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos(allShaderCount);

	uint32_t stageCreateInfoIndex = 0;
	SetShaderStageCreateInfo(SHADER_GROUP_TYPE_RAY_GEN, shaderStageCreateInfos, stageCreateInfoIndex);
	SetShaderStageCreateInfo(SHADER_GROUP_TYPE_MISS, shaderStageCreateInfos, stageCreateInfoIndex);
	SetShaderStageCreateInfo(SHADER_GROUP_TYPE_HIT, shaderStageCreateInfos, stageCreateInfoIndex);
	SetShaderStageCreateInfo(SHADER_GROUP_TYPE_CALLABLE, shaderStageCreateInfos, stageCreateInfoIndex);

	m_rayGenShaderGroupCount = gShaderContainer.GetShaderCount(SHADER_GROUP_TYPE_RAY_GEN);
	m_missShaderGroupCount = gShaderContainer.GetShaderCount(SHADER_GROUP_TYPE_MISS);
	m_hitShaderGroupCount = gHitGroupContainer.GetHitGroupCount();
	m_callableShaderGroupCount = gShaderContainer.GetShaderCount(SHADER_GROUP_TYPE_CALLABLE);
	m_shaderGroupCount = m_rayGenShaderGroupCount + m_missShaderGroupCount + m_hitShaderGroupCount + m_callableShaderGroupCount;

	m_shaderGroupCreateInfos.resize(m_shaderGroupCount);

	uint32_t groupCreateInfos = 0;

	for (uint32_t i = 0; i < m_rayGenShaderGroupCount; i++, groupCreateInfos++)
	{
		SimpleShader* shader = gShaderContainer.GetShader(SHADER_GROUP_TYPE_RAY_GEN, i);
		m_shaderGroupCreateInfos[groupCreateInfos].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		m_shaderGroupCreateInfos[groupCreateInfos].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		m_shaderGroupCreateInfos[groupCreateInfos].generalShader = gShaderContainer.GetBindIndex(shader);
		m_shaderGroupCreateInfos[groupCreateInfos].closestHitShader = VK_SHADER_UNUSED_KHR;
		m_shaderGroupCreateInfos[groupCreateInfos].anyHitShader = VK_SHADER_UNUSED_KHR;
		m_shaderGroupCreateInfos[groupCreateInfos].intersectionShader = VK_SHADER_UNUSED_KHR;
	}

	for (uint32_t i = 0; i < m_missShaderGroupCount; i++, groupCreateInfos++)
	{
		SimpleShader* shader = gShaderContainer.GetShader(SHADER_GROUP_TYPE_MISS, i);
		m_shaderGroupCreateInfos[groupCreateInfos].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		m_shaderGroupCreateInfos[groupCreateInfos].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		m_shaderGroupCreateInfos[groupCreateInfos].generalShader = gShaderContainer.GetBindIndex(shader);
		m_shaderGroupCreateInfos[groupCreateInfos].closestHitShader = VK_SHADER_UNUSED_KHR;
		m_shaderGroupCreateInfos[groupCreateInfos].anyHitShader = VK_SHADER_UNUSED_KHR;
		m_shaderGroupCreateInfos[groupCreateInfos].intersectionShader = VK_SHADER_UNUSED_KHR;
	}

	for (uint32_t i = 0; i < m_hitShaderGroupCount; i++, groupCreateInfos++)
	{
		RtHitShaderGroup* hitGroup = gHitGroupContainer.GetHitGroup(i);
		
		m_shaderGroupCreateInfos[groupCreateInfos].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		m_shaderGroupCreateInfos[groupCreateInfos].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		m_shaderGroupCreateInfos[groupCreateInfos].generalShader = VK_SHADER_UNUSED_KHR;
		m_shaderGroupCreateInfos[groupCreateInfos].closestHitShader = hitGroup->GetBindIndex(SHADER_TYPE_CLOSET_HIT);
		m_shaderGroupCreateInfos[groupCreateInfos].anyHitShader = hitGroup->GetBindIndex(SHADER_TYPE_ANY_HIT);
		m_shaderGroupCreateInfos[groupCreateInfos].intersectionShader = hitGroup->GetBindIndex(SHADER_TYPE_INTERSECTION);
	}

	for (uint32_t i = 0; i < m_callableShaderGroupCount; i++, groupCreateInfos++)
	{
		SimpleShader* shader = gShaderContainer.GetShader(SHADER_GROUP_TYPE_CALLABLE, i);
		m_shaderGroupCreateInfos[groupCreateInfos].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		m_shaderGroupCreateInfos[groupCreateInfos].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		m_shaderGroupCreateInfos[groupCreateInfos].generalShader = gShaderContainer.GetBindIndex(shader);
		m_shaderGroupCreateInfos[groupCreateInfos].closestHitShader = VK_SHADER_UNUSED_KHR;
		m_shaderGroupCreateInfos[groupCreateInfos].anyHitShader = VK_SHADER_UNUSED_KHR;
		m_shaderGroupCreateInfos[groupCreateInfos].intersectionShader = VK_SHADER_UNUSED_KHR;
	}

	VkPipelineLibraryCreateInfoKHR pipelineLibraryCreateInfo = {};
	pipelineLibraryCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;
	pipelineLibraryCreateInfo.pNext = NULL;
	pipelineLibraryCreateInfo.libraryCount = 0;
	pipelineLibraryCreateInfo.pLibraries = NULL;

	VkRayTracingPipelineCreateInfoKHR rayTracingPipeLineCreateInfo = {};
	rayTracingPipeLineCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	rayTracingPipeLineCreateInfo.stageCount = static_cast<uint32_t>(shaderStageCreateInfos.size());
	rayTracingPipeLineCreateInfo.pStages = shaderStageCreateInfos.data();
	rayTracingPipeLineCreateInfo.groupCount = static_cast<uint32_t>(m_shaderGroupCreateInfos.size());
	rayTracingPipeLineCreateInfo.pGroups = m_shaderGroupCreateInfos.data();
	rayTracingPipeLineCreateInfo.maxRecursionDepth = 4;
	rayTracingPipeLineCreateInfo.layout = pipelineLayout;
	rayTracingPipeLineCreateInfo.libraries = pipelineLibraryCreateInfo;

	if (vkCreateRayTracingPipelinesKHR(gLogicalDevice, VK_NULL_HANDLE, 1, &rayTracingPipeLineCreateInfo, nullptr, &m_pipeline) != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Pipeline create failed.");
		return false;
	}
	return true;
}

void RTPipeline::Destroy()
{
	if (m_pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(gLogicalDevice, m_pipeline, nullptr);
		m_pipeline = VK_NULL_HANDLE;
	}
}

void RTPipeline::SetShaderStageCreateInfo(ERTShaderGroupType groupType,
										  std::vector<VkPipelineShaderStageCreateInfo>& creatInfoBuffer,
										  uint32_t& stageCreateInfoIndex)
{
	uint32_t shaderCountInGroup = gShaderContainer.GetShaderCount(groupType);
	SimpleShader* curShader = nullptr;
	for (uint32_t i = 0; i < shaderCountInGroup; i++, stageCreateInfoIndex++)
	{
		curShader = gShaderContainer.GetShader(groupType, i);
		if (curShader != nullptr)
		{
			creatInfoBuffer[stageCreateInfoIndex].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			creatInfoBuffer[stageCreateInfoIndex].stage = VK_RAY_TRACING_SHADER_TYPE[curShader->GetShaderType()];
			creatInfoBuffer[stageCreateInfoIndex].module = curShader->GetShaderModule();
			creatInfoBuffer[stageCreateInfoIndex].pName = "main";
		}
	}
}