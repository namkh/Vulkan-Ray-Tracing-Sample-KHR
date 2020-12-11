#include "CommandBuffers.h"
#include "VulkanDeviceResources.h"	

bool CommandBufferBase::ResetCommandBuffer()
{
	if (vkResetCommandBuffer(m_commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT) == VkResult::VK_SUCCESS)
	{
		return true;
	}
	REPORT(EReportType::REPORT_TYPE_ERROR, "Command buffer reset(vkResetCommandBuffer) failed.");
	return false;
}

void CommandBufferBase::FreeCommandBuffers()
{
	if (m_isInitialized && m_commandBuffer != VK_NULL_HANDLE && m_commandPool != VK_NULL_HANDLE)
	{
		vkFreeCommandBuffers(gLogicalDevice, m_commandPool, 1, &m_commandBuffer);
	}
}

bool CommandBufferBase::AllocateCommandBuffer(VkCommandPool cmdPool)
{
	m_commandPool = cmdPool;

	VkCommandBufferAllocateInfo commandBufferAllocInfo = {};
	commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocInfo.pNext = nullptr;
	commandBufferAllocInfo.commandPool = m_commandPool;
	commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocInfo.commandBufferCount = 1;
	bool res = vkAllocateCommandBuffers(gLogicalDevice, &commandBufferAllocInfo, &m_commandBuffer) == VkResult::VK_SUCCESS;
	if (!res)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "VkCommandBuffer allocate(vkAllocateCommandBuffers) failed.");
	}
	return res;
}

bool CommandBufferBase::BeginCommandBuffer()
{
	ResetCommandBuffer();
	VkCommandBufferBeginInfo commandBufBegInfo = {};
	commandBufBegInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	return vkBeginCommandBuffer(m_commandBuffer, &commandBufBegInfo) == VkResult::VK_SUCCESS;
}

bool CommandBufferBase::EndCommandBuffer()
{
	return vkEndCommandBuffer(m_commandBuffer) == VkResult::VK_SUCCESS;
}

bool CommandBuffer::Initialize(VkCommandPool cmdPool)
{
	if (AllocateCommandBuffer(cmdPool))
	{
		m_isInitialized = true;
		return true;
	}
	return false;
}

bool CommandBuffer::Begin()
{
	vkResetCommandBuffer(m_commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	return BeginCommandBuffer();
}

bool CommandBuffer::End()
{
	m_state = COMMAND_BUFFER_STATE_RECORDED;
	return EndCommandBuffer();
}

bool StaticCommandBufferContainer::Initialize(VkCommandPool commandPool, uint32_t allocCount)
{
	if (commandPool == VK_NULL_HANDLE)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "StaticCommandBufferContainer initialize failed.(commandPool is VK_NULL_HANDLE)");
		return false;
	}

	m_commandPool = commandPool;
	m_commandList.resize(allocCount);
	for (auto& cur : m_commandList)
	{
		cur.Initialize(m_commandPool);
	}

	return true;
}

CommandBuffer* StaticCommandBufferContainer::GetCommandBuffer(uint32_t index)
{
	if (index < m_commandList.size())
	{
		return &m_commandList[index];
	}
	return nullptr;
}

void StaticCommandBufferContainer::Clear()
{
	for (auto& cur : m_commandList)
	{
		cur.FreeCommandBuffers();
	}
	m_commandList.clear();
}

void StaticCommandBufferContainer::Reset()
{
	for (auto& cur : m_commandList)
	{
		cur.ResetCommandBuffer();
	}
}

bool DynamicCommandBufferContainer::Initialize(VkCommandPool commandPool)
{
	if (commandPool != VK_NULL_HANDLE)
	{
		m_commandPool = commandPool;
		return true;
	}
	return false;
}

DynamicCommandBufferContainer::~DynamicCommandBufferContainer()
{
}

CommandBuffer* DynamicCommandBufferContainer::GetCommandBuffer()
{
	for (int i = 0; i < m_commandList.size(); i++)
	{
		if (m_commandList[i].GetState() == CommandBuffer::ECommandBufferState::COMMAND_BUFFER_STATE_READY)
		{
			return &m_commandList[i];
		}
	}

	m_commandList.emplace_back();
	m_commandList[m_commandList.size() - 1].Initialize(m_commandPool);
	return &m_commandList[m_commandList.size() - 1];

	return nullptr;
}

CommandBuffer* DynamicCommandBufferContainer::GetCommandBuffer(uint32_t index)
{
	if (index < m_commandList.size())
	{
		if (m_commandList[index].GetState() == CommandBuffer::COMMAND_BUFFER_STATE_READY)
		{
			return &m_commandList[index];
		}
		else
		{
			//인덱스에 해당하는 커맨드 버퍼가 준비되지 않았음을 로깅
		}
	}
	return nullptr;
}

void DynamicCommandBufferContainer::Clear()
{
	for (auto& cur : m_commandList)
	{
		cur.FreeCommandBuffers();
	}
	m_commandList.clear();
}

bool SingleTimeCommandBuffer::Begin()
{
	VkCommandBufferAllocateInfo commandBufferAllocInfo = {};
	commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocInfo.pNext = nullptr;
	//일단 디폴트 커맨드 풀을 쓰자
	//나중에는 디바이스에서 풀링할지 어떨지는 멀티스레드 렌더링 이후 생각해보자
	commandBufferAllocInfo.commandPool = VulkanDeviceResources::Instance().GetDefaultCommandPool();
	commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocInfo.commandBufferCount = 1;

	if (AllocateCommandBuffer(VulkanDeviceResources::Instance().GetDefaultCommandPool()))
	{
		VkCommandBufferBeginInfo commandBufBegInfo = {};
		commandBufBegInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		if (BeginCommandBuffer())
		{
			m_isInitialized = true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	return true;
}

bool SingleTimeCommandBuffer::End()
{
	if (EndCommandBuffer())
	{
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		VkFence fence = VK_NULL_HANDLE;
		vkCreateFence(gLogicalDevice, &fenceCreateInfo, nullptr, &fence);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = 0;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_commandBuffer;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		if (vkQueueSubmit(VulkanDeviceResources::Instance().GetGraphicsQueue(), 1, &submitInfo, fence) == VkResult::VK_SUCCESS)
		{
			VkResult res;
			do
			{
				res = vkWaitForFences(gLogicalDevice, 1, &fence, VK_TRUE, FENCE_TIMEOUT);
			} while (res == VK_TIMEOUT);
			if (res != VkResult::VK_SUCCESS)
			{
				if (res == VkResult::VK_ERROR_DEVICE_LOST)
				{
					REPORT(EReportType::REPORT_TYPE_ERROR, "Queue submit failed.(Device Lost)");
				}
				return false;
			}
			FreeCommandBuffers();
			vkDestroyFence(gLogicalDevice, fence, nullptr);
		}
		else
		{
			//queue submit 실패 로깅
			return false;
		}

	}
	else
	{
		//커맨드 버퍼 end 실패 로깅
		return false;
	}
	return true;
}