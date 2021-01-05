
#include "SimpleTexture.h"
#include "VulkanDeviceResources.h"
#include "CommandBuffers.h"

#include <stb_image.h>

SimpleTexture2D::SimpleTexture2D()
{

}

bool SimpleTexture2D::Load(const char* filePath)
{
	m_srcFilePath = filePath;

	int texChannels = 0;
	stbi_uc* pixels = stbi_load(filePath, &m_width, &m_height, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = static_cast<uint32_t>(m_width)
								* static_cast<uint32_t>(m_height)
								* 4;

	if (pixels == nullptr)
	{
		//stb image load 실패로깅
		return false;
	}

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = nullptr;
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.size = imageSize;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 0;
	bufferCreateInfo.pQueueFamilyIndices = nullptr;

	VkBuffer stagingBuffer = nullptr;
	VkDeviceMemory stagingBufferMemory = nullptr;
	if (vkCreateBuffer(gLogicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer) != VkResult::VK_SUCCESS)
	{
		//이미지 스테이징 버퍼 생성실패 로깅
		return false;
	}

	VkMemoryRequirements bufferMemRequirements = {};
	vkGetBufferMemoryRequirements(gLogicalDevice, stagingBuffer, &bufferMemRequirements);

	uint32_t memoryTypeIndex = 0;
	if (!VulkanDeviceResources::Instance().MemoryTypeFromProperties(bufferMemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &memoryTypeIndex) != VkResult::VK_SUCCESS)
	{
		//이미지 스테이징 버퍼 메모리 타입 취득 실패로깅
		return false;
	}

	VkMemoryAllocateInfo memAllocInfo = {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.pNext = nullptr;
	memAllocInfo.allocationSize = bufferMemRequirements.size;
	memAllocInfo.memoryTypeIndex = memoryTypeIndex;

	if (vkAllocateMemory(gLogicalDevice, &memAllocInfo, nullptr, &stagingBufferMemory) != VkResult::VK_SUCCESS)
	{
		//이미지 스테이징 메모리 할당실패 로깅
		return false;
	}

	if (vkBindBufferMemory(gLogicalDevice, stagingBuffer, stagingBufferMemory, 0))
	{
		//스테이징 리소스 메모리 바인드 실패 로깅 
		return false;
	}

	void* data = nullptr;
	if (vkMapMemory(gLogicalDevice, stagingBufferMemory, 0, imageSize, 0, &data) != VkResult::VK_SUCCESS)
	{
		//이미지 스테이징 버퍼 메모리 맵 실패
		return false;
	}
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(gLogicalDevice, stagingBufferMemory);

	stbi_image_free(pixels);

	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = nullptr;
	imageCreateInfo.flags = 0;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageCreateInfo.extent.width = static_cast<uint32_t>(m_width);
	imageCreateInfo.extent.height = static_cast<uint32_t>(m_height);
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 8;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (vkCreateImage(gLogicalDevice, &imageCreateInfo, nullptr, &m_image) != VkResult::VK_SUCCESS)
	{
		//이미지 생성 실패 로깅
		return false;
	}

	VkMemoryRequirements imageMemRequirements = {};
	vkGetImageMemoryRequirements(gLogicalDevice, m_image, &imageMemRequirements);

	memoryTypeIndex = 0;
	if (!VulkanDeviceResources::Instance().MemoryTypeFromProperties(imageMemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryTypeIndex))
	{
		//이미지 메모리 타입 검색 실패를 로깅
		return false;
	}

	memAllocInfo.allocationSize = imageMemRequirements.size;
	memAllocInfo.memoryTypeIndex = memoryTypeIndex;

	if (vkAllocateMemory(gLogicalDevice, &memAllocInfo, nullptr, &m_memory) != VkResult::VK_SUCCESS)
	{
		//이미지 메모리 할당 실패를 로깅
		return false;
	}

	vkBindImageMemory(gLogicalDevice, m_image, m_memory, 0);


	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.pNext = nullptr;
	imageMemoryBarrier.srcAccessMask = 0;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.image = m_image;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount = 1;

	SingleTimeCommandBuffer cmdBuffer;
	cmdBuffer.Begin();

	vkCmdPipelineBarrier(cmdBuffer.GetCommandBuffer(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height), 1 };

	vkCmdCopyBufferToImage(cmdBuffer.GetCommandBuffer(), stagingBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vkCmdPipelineBarrier(cmdBuffer.GetCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

	cmdBuffer.End();

	vkDestroyBuffer(gLogicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(gLogicalDevice, stagingBufferMemory, nullptr);

	VkImageViewCreateInfo imageViewCreateInfo = {};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.pNext = nullptr;
	imageViewCreateInfo.flags = 0;
	imageViewCreateInfo.image = m_image;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(gLogicalDevice, &imageViewCreateInfo, nullptr, &m_imageView) != VkResult::VK_SUCCESS)
	{
		//이미지 뷰 생성실패
		return false;
	}

	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.pNext = nullptr;
	samplerCreateInfo.flags = 0;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	samplerCreateInfo.maxAnisotropy = 16.0f;
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = 0.0f;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

	if (vkCreateSampler(gLogicalDevice, &samplerCreateInfo, nullptr, &m_imageSampler) != VkResult::VK_SUCCESS)
	{
		//샘플러 생성실패 로깅
		return false;
	}

	m_imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_imageInfo.imageView = m_imageView;
	m_imageInfo.sampler = m_imageSampler;

	return true;
}

void SimpleTexture2D::Destroy()
{
	DecRef();
}

void SimpleTexture2D::Unload()
{
	gVkDeviceRes.GraphicsQueueWaitIdle();

	if (m_imageSampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(gLogicalDevice, m_imageSampler, nullptr);
	}
	if (m_imageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(gLogicalDevice, m_imageView, nullptr);
	}
	if (m_image != VK_NULL_HANDLE)
	{
		vkDestroyImage(gLogicalDevice, m_image, nullptr);
	}
	if (m_memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(gLogicalDevice, m_memory, nullptr);
	}
}

SimpleCubmapTexture::SimpleCubmapTexture()
{

}

bool SimpleCubmapTexture::Initialize(std::vector<std::string>& filePath)
{
	int texChannels = 0;
	stbi_uc* pixelBuffer[6] = {};
	for (int i = 0; i < filePath.size(); i++)
	{
		pixelBuffer[i] = stbi_load(filePath[i].c_str(), &m_width, &m_height, &texChannels, STBI_rgb_alpha);
		if (pixelBuffer[i] == nullptr)
		{
			//stb image load 실패로깅
			//그냥 assert?
			return false;
		}
	}

	VkDeviceSize layerSize = static_cast<uint64_t>(m_width * m_height * 4);
	VkDeviceSize imageSize = layerSize * 6;

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = nullptr;
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.size = imageSize;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 0;
	bufferCreateInfo.pQueueFamilyIndices = nullptr;

	VkBuffer stagingBuffer = nullptr;
	VkDeviceMemory stagingBufferMemory = nullptr;
	if (vkCreateBuffer(gLogicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer) != VkResult::VK_SUCCESS)
	{
		//이미지 스테이징 버퍼 생성실패 로깅
		return false;
	}

	VkMemoryRequirements bufferMemRequirements = {};
	vkGetBufferMemoryRequirements(gLogicalDevice, stagingBuffer, &bufferMemRequirements);

	uint32_t memoryTypeIndex = 0;
	if (!VulkanDeviceResources::Instance().MemoryTypeFromProperties(bufferMemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &memoryTypeIndex) != VkResult::VK_SUCCESS)
	{
		//이미지 스테이징 버퍼 메모리 타입 취득 실패로깅
		return false;
	}

	VkMemoryAllocateInfo memAllocInfo = {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.pNext = nullptr;
	memAllocInfo.allocationSize = bufferMemRequirements.size;
	memAllocInfo.memoryTypeIndex = memoryTypeIndex;

	if (vkAllocateMemory(gLogicalDevice, &memAllocInfo, nullptr, &stagingBufferMemory) != VkResult::VK_SUCCESS)
	{
		//이미지 스테이징 메모리 할당실패 로깅
		return false;
	}

	if (vkBindBufferMemory(gLogicalDevice, stagingBuffer, stagingBufferMemory, 0))
	{
		//스테이징 리소스 메모리 바인드 실패 로깅 
		return false;
	}

	uint8_t* data = nullptr;
	if (vkMapMemory(gLogicalDevice, stagingBufferMemory, 0, imageSize, 0, (void**)&data) != VkResult::VK_SUCCESS)
	{
		//이미지 스테이징 버퍼 메모리 맵 실패
		return false;
	}
	for (int i = 0; i < 6; i++)
	{
		memcpy(data + (i * layerSize), pixelBuffer[i], layerSize);
	}
	vkUnmapMemory(gLogicalDevice, stagingBufferMemory);

	for (int i = 0; i < 6; i++)
	{
		stbi_image_free(pixelBuffer[i]);
	}

	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = nullptr;
	imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageCreateInfo.extent.width = static_cast<uint32_t>(m_width);
	imageCreateInfo.extent.height = static_cast<uint32_t>(m_height);
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 6;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (vkCreateImage(gLogicalDevice, &imageCreateInfo, nullptr, &m_image) != VkResult::VK_SUCCESS)
	{
		//이미지 생성 실패 로깅
		return false;
	}

	VkMemoryRequirements imageMemRequirements = {};
	vkGetImageMemoryRequirements(gLogicalDevice, m_image, &imageMemRequirements);

	memoryTypeIndex = 0;
	if (!VulkanDeviceResources::Instance().MemoryTypeFromProperties(imageMemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryTypeIndex))
	{
		//이미지 메모리 타입 검색 실패를 로깅
		return false;
	}

	memAllocInfo.allocationSize = imageMemRequirements.size;
	memAllocInfo.memoryTypeIndex = memoryTypeIndex;

	if (vkAllocateMemory(gLogicalDevice, &memAllocInfo, nullptr, &m_memory) != VkResult::VK_SUCCESS)
	{
		//이미지 메모리 할당 실패를 로깅
		return false;
	}

	vkBindImageMemory(gLogicalDevice, m_image, m_memory, 0);


	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.pNext = nullptr;
	imageMemoryBarrier.srcAccessMask = 0;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.image = m_image;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount = 6;

	SingleTimeCommandBuffer cmdBuffer;
	cmdBuffer.Begin();

	vkCmdPipelineBarrier(cmdBuffer.GetCommandBuffer(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 6;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height), 1 };

	vkCmdCopyBufferToImage(cmdBuffer.GetCommandBuffer(), stagingBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vkCmdPipelineBarrier(cmdBuffer.GetCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

	cmdBuffer.End();

	vkDestroyBuffer(gLogicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(gLogicalDevice, stagingBufferMemory, nullptr);

	VkImageViewCreateInfo imageViewCreateInfo = {};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.pNext = nullptr;
	imageViewCreateInfo.flags = 0;
	imageViewCreateInfo.image = m_image;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 6;

	if (vkCreateImageView(gLogicalDevice, &imageViewCreateInfo, nullptr, &m_imageView) != VkResult::VK_SUCCESS)
	{
		//이미지 뷰 생성실패
		return false;
	}

	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.pNext = nullptr;
	samplerCreateInfo.flags = 0;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	samplerCreateInfo.maxAnisotropy = 16.0f;
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = 0.0f;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

	if (vkCreateSampler(gLogicalDevice, &samplerCreateInfo, nullptr, &m_imageSampler) != VkResult::VK_SUCCESS)
	{
		//샘플러 생성실패 로깅
		return false;
	}

	m_imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_imageInfo.imageView = m_imageView;
	m_imageInfo.sampler = m_imageSampler;

	return true;
}

void SimpleCubmapTexture::Destroy()
{
	vkDestroySampler(gLogicalDevice, m_imageSampler, nullptr);
	vkDestroyImageView(gLogicalDevice, m_imageView, nullptr);
	vkDestroyImage(gLogicalDevice, m_image, nullptr);
	vkFreeMemory(gLogicalDevice, m_memory, nullptr);
}

bool RtTargetImageBuffer::Initialize(uint32_t width, uint32_t height, VkFormat format)
{
	m_width = width;
	m_height = height;
	m_format = format;

	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = m_format;
	imageCreateInfo.extent.width = static_cast<uint32_t>(m_width);
	imageCreateInfo.extent.height = static_cast<uint32_t>(m_height);
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (vkCreateImage(gLogicalDevice, &imageCreateInfo, nullptr, &m_image) != VkResult::VK_SUCCESS)
	{
		//레이트레이싱 타깃 이미지 생성 실패 로깅
		return false;
	}

	VkMemoryRequirements memRequirements = {};
	vkGetImageMemoryRequirements(gLogicalDevice, m_image, &memRequirements);

	uint32_t memoryTypeIndex = 0;
	if (!VulkanDeviceResources::Instance().MemoryTypeFromProperties(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryTypeIndex) != VkResult::VK_SUCCESS)
	{
		//이미지 스테이징 버퍼 메모리 타입 취득 실패로깅
		return false;
	}

	VkMemoryAllocateInfo memAllocInfo = {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = memRequirements.size;
	memAllocInfo.memoryTypeIndex = memoryTypeIndex;

	if (vkAllocateMemory(gLogicalDevice, &memAllocInfo, nullptr, &m_memory) != VkResult::VK_SUCCESS)
	{
		//레이트레이싱 타깃 이미지 메모리 할당실패 로깅
		return false;
	}

	if (vkBindImageMemory(gLogicalDevice, m_image, m_memory, 0) != VkResult::VK_SUCCESS)
	{
		//레이트레이싱 타깃 이미지 메모리 바인드 실패 로깅 
		return false;
	}

	VkImageViewCreateInfo imageViewCreateInfo = {};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.pNext = nullptr;
	imageViewCreateInfo.flags = 0;
	imageViewCreateInfo.image = m_image;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = m_format;
	imageViewCreateInfo.subresourceRange = {};
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(gLogicalDevice, &imageViewCreateInfo, nullptr, &m_imageView) != VkResult::VK_SUCCESS)
	{
		//이미지 뷰 생성실패
		return false;
	}

	m_imageInfo.imageView = m_imageView;
	m_imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkImageMemoryBarrier imageMemBarrier = {};
	imageMemBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemBarrier.srcAccessMask = 0;
	imageMemBarrier.dstAccessMask = 0;
	imageMemBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageMemBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

	imageMemBarrier.image = m_image;
	imageMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageMemBarrier.subresourceRange.baseMipLevel = 0;
	imageMemBarrier.subresourceRange.levelCount = 1;
	imageMemBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemBarrier.subresourceRange.layerCount = 1;

	SingleTimeCommandBuffer singleTimeCommandBuffer;
	singleTimeCommandBuffer.Begin();

	vkCmdPipelineBarrier
	(
		singleTimeCommandBuffer.GetCommandBuffer(),
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&imageMemBarrier
	);

	singleTimeCommandBuffer.End();

	return true;
}

void RtTargetImageBuffer::Destroy()
{
	if (m_imageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(gLogicalDevice, m_imageView, nullptr);
		m_imageView = VK_NULL_HANDLE;
	}

	if (m_memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(gLogicalDevice, m_memory, nullptr);
		m_memory = VK_NULL_HANDLE;
	}
	if (m_image != VK_NULL_HANDLE)
	{
		vkDestroyImage(gLogicalDevice, m_image, nullptr);
		m_image = VK_NULL_HANDLE;
	}
}

void RtTargetImageBuffer::Resize(uint32_t width, uint32_t height)
{
	Destroy();
	Initialize(width, height, m_format);
}