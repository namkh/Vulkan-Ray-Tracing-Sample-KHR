#pragma once

#include "DeviceBuffers.h"
#include "RTPipeline.h"

class RTShaderBindingTable
{
public:
	bool Build(RTPipeline* pipeline);
	bool Refresh();
	void Destory();

	VkStridedDeviceAddressRegionKHR* GetStridedBufferRegion(ERTShaderGroupType groupType);

protected:
	bool CreateShaderBindingTable(ERTShaderGroupType groupType, uint32_t shaderGroupCount, std::vector<uint8_t>& shaderHandleBuffer, uint32_t offset, BufferData* buffer);

private:
	BufferData m_raygenShaderBindingTableBuffer = {};
	BufferData m_missShaderBindingTableBuffer = {};
	BufferData m_hitShaderBindingTableBuffer = {};
	BufferData m_callableShaderBindingTableBuffer = {};

	RTPipeline* m_pipeline = nullptr;

	VkStridedDeviceAddressRegionKHR  m_shaderSbtEntries[SHADER_GROUP_TYPE_END] = {{}, {}, {}, {}};
};

