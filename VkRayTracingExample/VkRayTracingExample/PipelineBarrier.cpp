
#include "PipelineBarrier.h"

void PipelineBarrier::Write()
{
	std::vector<VkImageMemoryBarrier> imageBarriers;
	std::vector<VkBufferMemoryBarrier> bufferBarriers;

	for (auto& cur : m_imageMemBarriers)
	{
		imageBarriers.push_back(cur.second);
	}
	m_imageMemBarriers.clear();

	for (auto& cur : m_bufferMemBarriers)
	{
		bufferBarriers.push_back(cur.second);
	}
	m_bufferMemBarriers.clear();

	vkCmdPipelineBarrier
	(
		m_cmdBuf,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		0,
		0,
		nullptr,
		static_cast<uint32_t>(bufferBarriers.size()),
		bufferBarriers.data(),
		static_cast<uint32_t>(imageBarriers.size()),
		imageBarriers.data()
	);
}

void PipelineBarrier::SetAccessMask(VkAccessFlags oldMask, VkAccessFlags newMask, VkImage image)
{
	VkImageMemoryBarrier* barrier = GetBarrier(image);
	barrier->srcAccessMask = oldMask;
	barrier->dstAccessMask = newMask;
}

void PipelineBarrier::SetLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkImage image)
{
	VkImageMemoryBarrier* barrier = GetBarrier(image);
	barrier->oldLayout = oldLayout;
	barrier->newLayout = newLayout;
}

void PipelineBarrier::SetQueueFamilyIndex(uint32_t oldIdx, uint32_t newIdx, VkImage image)
{
	VkImageMemoryBarrier* barrier = GetBarrier(image);
	barrier->srcQueueFamilyIndex = oldIdx;
	barrier->dstAccessMask = newIdx;
}

void PipelineBarrier::SetImageSubresouceRange(VkImageSubresourceRange& subResourceRange, VkImage image)
{
	VkImageMemoryBarrier* barrier = GetBarrier(image);
	barrier->subresourceRange = subResourceRange;
}

void PipelineBarrier::SetAccessMask(VkAccessFlags oldMask, VkAccessFlags newMask, VkBuffer buffer)
{
	VkBufferMemoryBarrier* barrier = GetBarrier(buffer);
	barrier->srcAccessMask = oldMask;
	barrier->dstAccessMask = newMask;
}

void PipelineBarrier::SetQueueFamilyIndex(uint32_t oldIdx, uint32_t newIdx, VkBuffer buffer)
{
	VkBufferMemoryBarrier* barrier = GetBarrier(buffer);
	barrier->srcQueueFamilyIndex = oldIdx;
	barrier->dstAccessMask = newIdx;
}

void PipelineBarrier::SetBufferResourceRange(BufferResouceRange& range, VkBuffer buffer)
{
	VkBufferMemoryBarrier* barrier = GetBarrier(buffer);
	barrier->offset = range.Offset;
	barrier->size = range.Size;
}

VkImageMemoryBarrier* PipelineBarrier::GetBarrier(VkImage image)
{
	VkImageMemoryBarrier* barrier = nullptr;
	auto iterFind = m_imageMemBarriers.find(image);
	if (iterFind != m_imageMemBarriers.end())
	{
		barrier = &iterFind->second;
	}
	else
	{
		m_imageMemBarriers.insert(std::make_pair(image, VkImageMemoryBarrier() = {}));
		barrier = &m_imageMemBarriers[image];
		barrier->image = image;
		barrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	}
	return barrier;
}

VkBufferMemoryBarrier* PipelineBarrier::GetBarrier(VkBuffer buffer)
{
	VkBufferMemoryBarrier* barrier = nullptr;
	auto iterFind = m_bufferMemBarriers.find(buffer);
	if (iterFind != m_bufferMemBarriers.end())
	{
		barrier = &iterFind->second;
	}
	else
	{
		m_bufferMemBarriers.insert(std::make_pair(buffer, VkBufferMemoryBarrier() = {}));
		barrier = &m_bufferMemBarriers[buffer];
		barrier->buffer = buffer;
		barrier->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	}
	return barrier;
}