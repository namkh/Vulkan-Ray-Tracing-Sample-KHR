#pragma once

#include "DeviceBuffers.h"
#include "RTPipeline.h"

class RTShaderBindingTable
{
public:
	bool Build(RTPipeline* pipeline);
	bool Refresh();
	void Destory();

	BufferData* GetBuffer() { return &m_shaderBindingTableBuffer; }
	VkStridedBufferRegionKHR* GetStridedBufferRegion(ERTShaderGroupType groupType);
private:

	BufferData m_shaderBindingTableBuffer = {};
	RTPipeline* m_pipeline = nullptr;

	VkStridedBufferRegionKHR m_shaderSbtEntries[SHADER_GROUP_TYPE_END] = {{}, {}, {}, {}};
};

