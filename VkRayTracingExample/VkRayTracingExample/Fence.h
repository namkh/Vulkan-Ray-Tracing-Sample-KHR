#pragma once

#include "CommandBuffers.h"
#include <vector>

class Fence
{
public:
	bool Initialize();
	bool WaitForFence();
	bool Reset();
	void AddSubmittedCommandBuffer(CommandBuffer* cmdBuffer);
	void Destory();

	VkFence GetFence() { return m_fence; }

public:

	VkFence m_fence;
	std::vector<CommandBuffer*> m_submittedCommandBuffer;
};

