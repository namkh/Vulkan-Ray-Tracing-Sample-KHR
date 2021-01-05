#include "RTShaderBindingTable.h"

bool RTShaderBindingTable::Build(RTPipeline* pipeline)
{
	if (pipeline == nullptr)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "The pipeline was not created.");
		return false;
	}

	m_pipeline = pipeline;

	Refresh();
	return true;
}

bool RTShaderBindingTable::CreateShaderBindingTable(ERTShaderGroupType groupType, uint32_t shaderGroupCount, std::vector<uint8_t>& shaderHandleBuffer, uint32_t offset, BufferData* buffer)
{
	uint32_t shaderGroupHandleSize = VulkanDeviceResources::Instance().GetPhysicalDeviceRayTracingPipelineProperties().shaderGroupHandleSize;
	uint32_t shaderGroupBaseAlignment = VulkanDeviceResources::Instance().GetPhysicalDeviceRayTracingPipelineProperties().shaderGroupBaseAlignment;

	if (shaderGroupCount > 0)
	{
		uint32_t shaderBindingTableSize = shaderGroupBaseAlignment * shaderGroupCount;
		if (buffer->IsAllocated())
		{
			if (!buffer->Resize(shaderBindingTableSize))
			{
				return false;
			}
		}
		else
		{
			if (!buffer->Initialize(shaderBindingTableSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
			{
				return false;
			}
		}
		
		for (uint32_t i = 0; i < shaderGroupCount; i++)
		{
			if (!buffer->UpdateResource(shaderHandleBuffer.data() + offset + i * shaderGroupHandleSize, i * shaderGroupBaseAlignment, shaderGroupHandleSize))
			{
				return false;
			}
		}

		m_shaderSbtEntries[groupType].deviceAddress = buffer->GetDeviceMemoryAddress();
		m_shaderSbtEntries[groupType].stride = shaderGroupBaseAlignment;
		m_shaderSbtEntries[groupType].size = shaderBindingTableSize;
	}
	return true;
}

bool RTShaderBindingTable::Refresh()
{
	uint32_t shaderGroupCount = m_pipeline->GetShaderGroupCount();
	uint32_t shaderGroupHandleSize = VulkanDeviceResources::Instance().GetPhysicalDeviceRayTracingPipelineProperties().shaderGroupHandleSize;
	uint32_t shaderGroupBaseAlignment = VulkanDeviceResources::Instance().GetPhysicalDeviceRayTracingPipelineProperties().shaderGroupBaseAlignment;
	uint32_t shaderBindingHandleBufferSize = shaderGroupHandleSize * shaderGroupCount;

	std::vector<uint8_t> shaderHandleBuffer(shaderBindingHandleBufferSize);
	if (vkGetRayTracingShaderGroupHandlesKHR(gLogicalDevice, m_pipeline->GetPipeline(), 0, shaderGroupCount, shaderBindingHandleBufferSize, shaderHandleBuffer.data()) != VkResult::VK_SUCCESS)
	{
		return false;
	}

	uint32_t offset = 0;
	uint32_t raygenShaderGroupCount = m_pipeline->GetRayGenShaderGroupCount();
	CreateShaderBindingTable(SHADER_GROUP_TYPE_RAY_GEN, raygenShaderGroupCount, shaderHandleBuffer, offset, &m_raygenShaderBindingTableBuffer);
	
	offset += shaderGroupHandleSize * raygenShaderGroupCount;
	uint32_t missShaderGroupCount = m_pipeline->GetMissShaderGroupCount();
	CreateShaderBindingTable(SHADER_GROUP_TYPE_MISS, missShaderGroupCount, shaderHandleBuffer, offset, &m_missShaderBindingTableBuffer);
	
	offset += shaderGroupHandleSize * missShaderGroupCount;
	uint32_t hitShaderGroupCount = m_pipeline->GetHitShaderGroupCount();
	CreateShaderBindingTable(SHADER_GROUP_TYPE_HIT, hitShaderGroupCount, shaderHandleBuffer, offset, &m_hitShaderBindingTableBuffer);
	
	offset += shaderGroupHandleSize * hitShaderGroupCount;
	uint32_t callableShaderGroupCount = m_pipeline->GetCallableShaderGroupCount();
	CreateShaderBindingTable(SHADER_GROUP_TYPE_CALLABLE, callableShaderGroupCount, shaderHandleBuffer, offset, &m_callableShaderBindingTableBuffer);
	
	return true;
}

void RTShaderBindingTable::Destory()
{
	if (m_raygenShaderBindingTableBuffer.IsAllocated())
	{
		m_raygenShaderBindingTableBuffer.Destroy();
	}

	if (m_missShaderBindingTableBuffer.IsAllocated())
	{
		m_missShaderBindingTableBuffer.Destroy();
	}

	if (m_hitShaderBindingTableBuffer.IsAllocated())
	{
		m_hitShaderBindingTableBuffer.Destroy();
	}

	if (m_callableShaderBindingTableBuffer.IsAllocated())
	{
		m_callableShaderBindingTableBuffer.Destroy();
	}
}

VkStridedDeviceAddressRegionKHR* RTShaderBindingTable::GetStridedBufferRegion(ERTShaderGroupType groupType)
{
	if (groupType != SHADER_GROUP_TYPE_INVALID && groupType != SHADER_GROUP_TYPE_END)
	{
		return &m_shaderSbtEntries[groupType];
	}
	return nullptr;
}

