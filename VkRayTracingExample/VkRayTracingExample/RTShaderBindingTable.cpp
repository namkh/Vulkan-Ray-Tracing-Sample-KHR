#include "RTShaderBindingTable.h"

bool RTShaderBindingTable::Build(RTPipeline* pipeline)
{
	if (pipeline == nullptr)
	{
		//파이프라인이 아직 생성되지 않았음을 로깅
		return false;
	}

	m_pipeline = pipeline;

	Refresh();
	return true;
}

bool RTShaderBindingTable::Refresh()
{
	uint32_t shaderGroupCount = m_pipeline->GetShaderGroupCount();
	uint32_t shaderGroupHandleSize = VulkanDeviceResources::Instance().GetPhysicalDeviceRayTracingProperties().shaderGroupHandleSize;
	uint32_t shaderGroupBaseAlignment = VulkanDeviceResources::Instance().GetPhysicalDeviceRayTracingProperties().shaderGroupBaseAlignment;
	uint32_t shaderBindingTableBufferSize = shaderGroupBaseAlignment * shaderGroupCount;
	uint32_t shaderBindingHandleBufferSize = shaderGroupHandleSize * shaderGroupCount;

	if (m_shaderBindingTableBuffer.IsAllocated())
	{
		if (!m_shaderBindingTableBuffer.Resize(shaderBindingTableBufferSize))
		{
			return false;
		}
	}
	else
	{
		if (!m_shaderBindingTableBuffer.Initialzie(shaderBindingTableBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
		{
			return false;
		}
	}

	std::vector<uint8_t> shaderHandleBuffer(shaderBindingHandleBufferSize);
	if (vkGetRayTracingShaderGroupHandlesKHR(gLogicalDevice, m_pipeline->GetPipeline(), 0, shaderGroupCount, shaderBindingHandleBufferSize, shaderHandleBuffer.data()) != VkResult::VK_SUCCESS)
	{
		return false;
	}

	for (uint32_t i = 0; i < shaderGroupCount; i++)
	{
		if (!m_shaderBindingTableBuffer.UpdateResource(shaderHandleBuffer.data() + (i * shaderGroupHandleSize), i * shaderGroupBaseAlignment, shaderGroupHandleSize))
		{
			return false;
		}
	}

	if (m_pipeline->GetRayGenShaderGroupCount() > 0)
	{
		m_shaderSbtEntries[SHADER_GROUP_TYPE_RAY_GEN].buffer = m_shaderBindingTableBuffer.GetBuffer();
		m_shaderSbtEntries[SHADER_GROUP_TYPE_RAY_GEN].offset = static_cast<VkDeviceSize>(0u * shaderGroupBaseAlignment);
		m_shaderSbtEntries[SHADER_GROUP_TYPE_RAY_GEN].stride = shaderGroupBaseAlignment;
		m_shaderSbtEntries[SHADER_GROUP_TYPE_RAY_GEN].size = shaderBindingTableBufferSize;
	}

	if (m_pipeline->GetMissShaderGroupCount() > 0)
	{
		m_shaderSbtEntries[SHADER_GROUP_TYPE_MISS].buffer = m_shaderBindingTableBuffer.GetBuffer();
		m_shaderSbtEntries[SHADER_GROUP_TYPE_MISS].offset = static_cast<VkDeviceSize>(m_pipeline->GetRayGenShaderGroupCount() * shaderGroupBaseAlignment);
		m_shaderSbtEntries[SHADER_GROUP_TYPE_MISS].stride = shaderGroupBaseAlignment;
		m_shaderSbtEntries[SHADER_GROUP_TYPE_MISS].size = shaderBindingTableBufferSize;
	}
	
	if (m_pipeline->GetHitShaderGroupCount() > 0)
	{
		m_shaderSbtEntries[SHADER_GROUP_TYPE_HIT].buffer = m_shaderBindingTableBuffer.GetBuffer();
		m_shaderSbtEntries[SHADER_GROUP_TYPE_HIT].offset = static_cast<VkDeviceSize>((m_pipeline->GetRayGenShaderGroupCount()
																						+ m_pipeline->GetMissShaderGroupCount())
																						* shaderGroupBaseAlignment);
		m_shaderSbtEntries[SHADER_GROUP_TYPE_HIT].stride = shaderGroupBaseAlignment;
		m_shaderSbtEntries[SHADER_GROUP_TYPE_HIT].size = shaderBindingTableBufferSize;
	}

	if (m_pipeline->GetHitShaderGroupCount() > 0)
	{
		m_shaderSbtEntries[SHADER_GROUP_TYPE_CALLABLE].buffer = m_shaderBindingTableBuffer.GetBuffer();
		m_shaderSbtEntries[SHADER_GROUP_TYPE_CALLABLE].offset = static_cast<VkDeviceSize>((m_pipeline->GetRayGenShaderGroupCount()
																							+ m_pipeline->GetMissShaderGroupCount()
																							+ m_pipeline->GetHitShaderGroupCount())
																							* shaderGroupBaseAlignment);
		m_shaderSbtEntries[SHADER_GROUP_TYPE_CALLABLE].stride = shaderGroupBaseAlignment;
		m_shaderSbtEntries[SHADER_GROUP_TYPE_CALLABLE].size = shaderBindingTableBufferSize;
	}

	return true;
}

void RTShaderBindingTable::Destory()
{
	if (m_shaderBindingTableBuffer.IsAllocated())
	{
		m_shaderBindingTableBuffer.Destroy();
	}
}

VkStridedBufferRegionKHR* RTShaderBindingTable::GetStridedBufferRegion(ERTShaderGroupType groupType)
{
	if (groupType != SHADER_GROUP_TYPE_INVALID && groupType != SHADER_GROUP_TYPE_END)
	{
		return &m_shaderSbtEntries[groupType];
	}
	return nullptr;
}

