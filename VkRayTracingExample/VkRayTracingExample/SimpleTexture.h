#pragma once

#include "Utils.h"

class SimpleTexture2D : public RefCounter, public UniqueIdentifier
{
	friend class TextureContainer;
public:
	SimpleTexture2D();

	void Destroy();

	VkImage& GetImage() { return m_image; }
	VkDeviceMemory& GetMemory() { return m_memory; }
	VkDescriptorImageInfo& GetImageInfo() { return m_imageInfo; }

	std::string GetSrcFilePath() { return m_srcFilePath; }

protected:
	bool Load(const char* filePath);
	void Unload();

protected:
	VkImage			m_image = VK_NULL_HANDLE;
	VkDeviceMemory	m_memory = VK_NULL_HANDLE;

	VkImageView	m_imageView = VK_NULL_HANDLE;
	VkSampler	m_imageSampler = VK_NULL_HANDLE;

	VkDescriptorImageInfo	m_imageInfo = {};

	int m_width = 0;
	int m_height = 0;

	std::string m_srcFilePath = "";
};

class SimpleCubmapTexture
{
public:

	SimpleCubmapTexture();

	bool Initialize(std::vector<std::string>& filePath);
	void Destroy();

	VkImage& GetImage() { return m_image; }
	VkDeviceMemory& GetMemory() { return m_memory; }
	VkDescriptorImageInfo& GetImageInfo() { return m_imageInfo; }

protected:
	VkImage			m_image = VK_NULL_HANDLE;
	VkDeviceMemory	m_memory = VK_NULL_HANDLE;

	VkImageView		m_imageView = VK_NULL_HANDLE;
	VkSampler		m_imageSampler = VK_NULL_HANDLE;

	VkDescriptorImageInfo	m_imageInfo = {};


	int						m_width = 0;
	int						m_height = 0;
};

class RtTargetImageBuffer
{
public:
	RtTargetImageBuffer() {}

public:
	bool Initialize(uint32_t width, uint32_t height, VkFormat format);
	void Destroy();

	void Resize(uint32_t width, uint32_t height);

public:

	VkImage& GetImage() { return m_image; }
	VkDeviceMemory& GetMemory() { return m_memory; }
	VkDescriptorImageInfo& GetImageInfo() { return m_imageInfo; }
	VkFormat& GetFormat() { return m_format; }

protected:
	VkImage			m_image = VK_NULL_HANDLE;
	VkDeviceMemory	m_memory = VK_NULL_HANDLE;
	VkImageView		m_imageView = VK_NULL_HANDLE;

	VkDescriptorImageInfo m_imageInfo = {};

	VkFormat m_format = VK_FORMAT_R8G8B8A8_UNORM;

	uint32_t m_width = 0;
	uint32_t m_height = 0;
};