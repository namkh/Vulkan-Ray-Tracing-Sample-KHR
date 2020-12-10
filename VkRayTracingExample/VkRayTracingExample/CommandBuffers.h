#pragma once

#include <vector>

#include "ExternalLib.h"
#include "Commands.h"

class CommandBufferBase
{
public:
	bool ResetCommandBuffer();
	void FreeCommandBuffers();

protected:
	bool AllocateCommandBuffer(VkCommandPool cmdPool);
	bool BeginCommandBuffer();
	bool EndCommandBuffer();

public:
	VkCommandBuffer GetCommandBuffer()
	{
		if (m_isInitialized)
		{
			return m_commandBuffer;
		}
		return VK_NULL_HANDLE;
	};

protected:

	VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
	VkCommandPool m_commandPool = VK_NULL_HANDLE;
	bool m_isInitialized = false;
};

class CommandBuffer : public CommandBufferBase
{
public:
	enum ECommandBufferState
	{
		COMMAND_BUFFER_STATE_READY,
		COMMAND_BUFFER_STATE_RECORDED,
		COMMAND_BUFFER_STATE_SUBMITTED,
	};
public:
	bool Initialize(VkCommandPool pool);
	bool Begin();
	bool End();

	ECommandBufferState GetState() { return m_state; }
	void OnSubmitted() { m_state = COMMAND_BUFFER_STATE_SUBMITTED; }
	void ExecutionComplete() { m_state = COMMAND_BUFFER_STATE_READY; }

private:

	ECommandBufferState m_state = COMMAND_BUFFER_STATE_READY;
};

class StaticCommandBufferContainer
{
public:
	bool Initialize(VkCommandPool commandPool, uint32_t allocCount);
	CommandBuffer* GetCommandBuffer(uint32_t index);
	void Clear();
	void Reset();

	uint32_t GetCommandBufferCount() { return static_cast<uint32_t>(m_commandList.size()); }

private:
	std::vector<CommandBuffer> m_commandList;
	VkCommandPool m_commandPool = VK_NULL_HANDLE;
};

class DynamicCommandBufferContainer
{
public:
	~DynamicCommandBufferContainer();
public:
	bool Initialize(VkCommandPool commandPool);
	CommandBuffer* GetCommandBuffer();
	CommandBuffer* GetCommandBuffer(uint32_t index);
	void Clear();

	uint32_t GetCommandBufferCount() { return static_cast<uint32_t>(m_commandList.size()); }

private:

	std::vector<CommandBuffer> m_commandList;
	VkCommandPool m_commandPool = VK_NULL_HANDLE;
};

//�������ʹ� command manager �Ǵ� ����̽� ���ҽ��κ��� ��ȯó���Ѵ�
//command pool�� command queue�� �޵���ó���ϴ°� ������??
//begin���� command pool�� �޴½�??
class SingleTimeCommandBuffer : public CommandBufferBase
{
public:
	bool Begin();
	bool End();
};