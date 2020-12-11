
#include "Fence.h"
#include "VulkanDeviceResources.h"

bool Fence::Initialize()
{
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = nullptr;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	if (vkCreateFence(gLogicalDevice, &fenceCreateInfo, nullptr, &m_fence) != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "fence create failed.");
		return false;
	}
	return true;
}

bool Fence::WaitForFence()
{
	VkResult res = VkResult::VK_SUCCESS;
	do
	{
		res = vkWaitForFences(gLogicalDevice, 1, &m_fence, VK_TRUE, UINT64_MAX);
	} while (res == VK_TIMEOUT);

	if (res != VkResult::VK_SUCCESS)
	{
		if (res == VkResult::VK_ERROR_DEVICE_LOST)
		{
			REPORT_WITH_SHUTDOWN(EReportType::REPORT_TYPE_ERROR, "Device Lost.");
		}
		return false;
	}
	else
	{
		for (auto& cur : m_submittedCommandBuffer)
		{
			cur->ExecutionComplete();
		}
		m_submittedCommandBuffer.clear();
	}

	return true;
}

bool Fence::Reset()
{
	return (vkResetFences(gLogicalDevice, 1, &m_fence) == VkResult::VK_SUCCESS);
}

void Fence::AddSubmittedCommandBuffer(CommandBuffer* cmdBuffer)
{
	m_submittedCommandBuffer.push_back(cmdBuffer);
}

void Fence::Destory()
{
	vkDestroyFence(gLogicalDevice, m_fence, nullptr);
}