#pragma once

#include "CommandBuffers.h"

class PipelineBarrier
{
public:
	PipelineBarrier(VkCommandBuffer cmdBuf) : m_cmdBuf(cmdBuf) {}

	void Write();

public:
	void SetAccessMask(VkAccessFlags oldFlag, VkAccessFlags newFlag, VkImage image);
	void SetLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkImage image);
	void SetQueueFamilyIndex(uint32_t oldIdx, uint32_t newIdx, VkImage image);
	void SetImageSubresouceRange(VkImageSubresourceRange& subResourceRange, VkImage image);

	void SetAccessMask(VkAccessFlags oldFlag, VkAccessFlags newFlag, VkBuffer buffer);
	void SetQueueFamilyIndex(uint32_t oldIdx, uint32_t newIdx, VkBuffer buffer);
	void SetBufferResourceRange(BufferResouceRange& range, VkBuffer buffer);

protected:

	VkImageMemoryBarrier* GetBarrier(VkImage image);
	VkBufferMemoryBarrier* GetBarrier(VkBuffer buffer);

public:
	std::map<VkImage, VkImageMemoryBarrier>		m_imageMemBarriers = {};
	std::map<VkBuffer, VkBufferMemoryBarrier>	m_bufferMemBarriers = {};

	VkCommandBuffer m_cmdBuf = VK_NULL_HANDLE;
};