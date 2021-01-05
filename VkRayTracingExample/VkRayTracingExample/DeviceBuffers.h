#pragma once

#include <stdlib.h>

#include "VulkanDeviceResources.h"
#include "CommandBuffers.h"

class BufferData
{
public:
	virtual bool Initialize(uint32_t size, VkBufferUsageFlags bufferUsage, VkFlags memRequirementsMask);
	bool Reset(uint32_t size, VkBufferUsageFlags bufferUsage, VkFlags memRequirementsMask);
	bool Resize(uint32_t size);
	bool UpdateResource(uint8_t* srcData, uint32_t offset, uint32_t size);
	void Destroy();

protected:
	virtual bool CreateBuffer();
	virtual bool AllocateMemory();

public:
	VkBuffer&				GetBuffer() { return m_buffer; }
	VkDeviceMemory&			GetMemory() { return m_memory; }
	VkDescriptorBufferInfo& GetBufferInfo() { return m_bufferInfo; }
	VkDeviceAddress			GetDeviceMemoryAddress() { return m_memoryAddress; }

	uint32_t GetBufferSize() { return m_size; }
	bool IsAllocated() { return m_isAllocated; }

public:
	VkBuffer				m_buffer = nullptr;
	VkBufferUsageFlags      m_bufferUsage = 0;
	VkFlags					m_memRequirementsMask = 0;
	VkDeviceMemory			m_memory = nullptr;
	VkDeviceAddress			m_memoryAddress = 0;
	VkDescriptorBufferInfo	m_bufferInfo = {};
	

	uint32_t m_size = 0;
	bool m_isAllocated = false;
};


template <typename ResourceType>
class StructuredBufferData
{
public:
	virtual bool Initialzie(VkBufferUsageFlags bufferUsage, VkFlags memRequirementsMask, uint32_t numDatas = 1);
	bool Reset(VkBufferUsageFlags bufferUsage, VkFlags memRequirementsMask, uint32_t numDatas = 1);
	bool Resize(uint32_t numDatas);
	virtual void Destroy();

protected:
	virtual bool CreateBuffer();
	virtual bool AllocateMemory();

public:
	virtual void UpdateResource(ResourceType* srcData, uint32_t bufferCount);
	virtual void UpdateResource(ResourceType& srcData, uint32_t index = 0);
	virtual void UpdateResource(ResourceType& srcData, std::vector<uint32_t>& updateIndices);

public:
	VkBuffer&				GetBuffer()			{ return m_buffer; }
	VkDeviceMemory&			GetMemory()			{ return m_memory; }
	VkDescriptorBufferInfo&	GetBufferInfo()		{ return m_bufferInfo; }
	uint32_t				GetStride()			{ return m_stride; }
	uint32_t				GetNumDatas()		{ return m_numDatas; }
	uint32_t				GetByteSize()		{ return m_byteSize; }
	VkDeviceAddress			GetDeviceMemoryAddress() { return m_memoryAddress; }

	bool IsAllocated() { return m_isAllocated; }

protected:
	VkBuffer				m_buffer = VK_NULL_HANDLE;
	VkDeviceMemory			m_memory = VK_NULL_HANDLE;
	VkDescriptorBufferInfo	m_bufferInfo = {};
	VkBufferUsageFlags		m_bufferUsage = 0;
	VkFlags					m_memRequirementsMask = 0;
	VkDeviceAddress			m_memoryAddress = 0;

	uint32_t m_numDatas = 0;
	uint32_t m_stride = 0;
	uint32_t m_byteSize = 0;

	bool m_isAllocated = false;
};

template <typename ResourceType>
bool StructuredBufferData<ResourceType>::Initialzie(VkBufferUsageFlags bufferUsage, VkFlags memRequirementsMask, uint32_t numDatas)
{
	if (m_isAllocated)
	{
		REPORT(EReportType::REPORT_TYPE_WARN, "BufferData is already allocated.");
		return false;
	}
	m_numDatas = numDatas;
	m_stride = sizeof(ResourceType);
	m_byteSize = m_stride * m_numDatas;
	m_bufferUsage = bufferUsage;
	m_memRequirementsMask = memRequirementsMask;

	if (m_numDatas < 1)
	{
		REPORT(EReportType::REPORT_TYPE_WARN, "The minimum size of an structured buffer is 1.");
		return false;
	}
	
	if (!CreateBuffer() || !AllocateMemory())
	{
		return false;
	}

	m_bufferInfo.buffer = m_buffer;
	m_bufferInfo.offset = 0;
	m_bufferInfo.range = m_byteSize;

	if ((m_bufferUsage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != 0)
	{
		m_memoryAddress = GetBufferDeviceAddress(m_buffer);
	}

	m_isAllocated = true;

	return true;
}

template <typename ResourceType>
bool StructuredBufferData<ResourceType>::Reset(VkBufferUsageFlags bufferUsage, VkFlags memRequirementsMask, uint32_t numDatas)
{
	m_bufferUsage = bufferUsage; 
	m_memRequirementsMask = bufferUsage;
	Resize(numDatas);

	return true;
}

template <typename ResourceType>
bool StructuredBufferData<ResourceType>::Resize(uint32_t numDatas)
{
	Destroy();
	if (!Initialzie(m_bufferUsage, m_memRequirementsMask, numDatas))
	{
		return false;
	}
	return true;
}

template <typename ResourceType>
void StructuredBufferData<ResourceType>::Destroy()
{
	if (m_buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(gLogicalDevice, m_buffer, nullptr);
		m_buffer = VK_NULL_HANDLE;
	}
	if (m_memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(gLogicalDevice, m_memory, nullptr);
		m_memory = VK_NULL_HANDLE;
	}
	m_isAllocated = false;
}

template <typename ResourceType>
bool StructuredBufferData<ResourceType>::CreateBuffer()
{
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = nullptr;
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.size = static_cast<VkDeviceSize>(m_byteSize);
	bufferCreateInfo.usage = m_bufferUsage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 0;
	bufferCreateInfo.pQueueFamilyIndices = nullptr;

	VkResult res = vkCreateBuffer(gLogicalDevice, &bufferCreateInfo, nullptr, &m_buffer);
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Buffer create failed.");
		return false;
	}
	return true;
}

template <typename ResourceType>
bool StructuredBufferData<ResourceType>::AllocateMemory()
{
	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(gLogicalDevice, m_buffer, &memReqs);

	VkMemoryAllocateFlagsInfo allocateFlagsInfo = {};
	allocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	allocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = 0;
	if ((m_bufferUsage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != 0)
	{
		allocInfo.pNext = &allocateFlagsInfo;
	}

	if (!VulkanDeviceResources::Instance().MemoryTypeFromProperties(memReqs.memoryTypeBits,
		m_memRequirementsMask,
		&allocInfo.memoryTypeIndex))
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Not found device meory type.");
		return false;
	}

	if (vkAllocateMemory(gLogicalDevice, &allocInfo, nullptr, &m_memory) != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Device memory allocation failed.");
		return false;
	}

	if (vkBindBufferMemory(gLogicalDevice, m_buffer, m_memory, 0) != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Buffer bind failed.");
		return false;
	}

	return true;
}

template <typename ResourceType>
void StructuredBufferData<ResourceType>::UpdateResource(ResourceType* srcData, uint32_t bufferCount)
{
	if (bufferCount == m_numDatas)
	{
		uint8_t* data = nullptr;
		VkResult res = vkMapMemory(gLogicalDevice, m_memory, 0, sizeof(ResourceType), 0, (void**)&data);
		if (res == VkResult::VK_SUCCESS)
		{
			memcpy(data, srcData, m_byteSize);
			vkUnmapMemory(gLogicalDevice, m_memory);
		}
		else
		{
			REPORT(EReportType::REPORT_TYPE_WARN, "Buffer memory map failed");
		}
	}
	else
	{
		REPORT(EReportType::REPORT_TYPE_WARN, "src buffer and dst buffer are have different size");
	}
}

template <typename ResourceType>
void StructuredBufferData<ResourceType>::UpdateResource(ResourceType& srcData, uint32_t dataIndex)
{
	if (dataIndex >= m_numDatas)
	{
		REPORT(EReportType::REPORT_TYPE_WARN, "An invalid index was requested.");
		return;
	}

	uint8_t* data = nullptr;
	VkResult res = vkMapMemory(gLogicalDevice, m_memory, 0, sizeof(ResourceType), 0, (void**)&data);
	if (res == VkResult::VK_SUCCESS)
	{
		data += sizeof(ResourceType) * dataIndex;
		memcpy(data, &srcData, sizeof(ResourceType));

		vkUnmapMemory(gLogicalDevice, m_memory);
	}
	else
	{
		REPORT(EReportType::REPORT_TYPE_WARN, "Buffer memory map failed");
	}
}

template <typename ResourceType>
void StructuredBufferData<ResourceType>::UpdateResource(ResourceType& srcData, std::vector<uint32_t>& updateIndices)
{
	for (int i = 0; i < updateIndices.size(); i++)
	{
		if (updateIndices[i] >= m_numDatas)
		{
			REPORT(EReportType::REPORT_TYPE_WARN, "An invalid index was requested.");
			return;
		}
	}

	uint8_t* data = nullptr;
	VkResult res = vkMapMemory(gLogicalDevice, m_memory, 0, sizeof(ResourceType), 0, (void**)&data);
	if (res == VkResult::VK_SUCCESS)
	{
		for (int i = 0; i < updateIndices.size(); i++)
		{
			data += sizeof(ResourceType) * updateIndices[i];
			memcpy(data, &srcData, m_stride);
		}
	}
	else
	{
		REPORT(EReportType::REPORT_TYPE_WARN, "Buffer memory map failed");
	}
	vkUnmapMemory(gLogicalDevice, m_memory);

}

struct DefaultVertex
{
	glm::vec4 m_position;
	glm::vec4 m_normal;
	glm::vec4 m_tangent;
	glm::vec4 m_color;
	glm::vec4 m_texcoord;
};

class VertexBuffer
{
public:
	virtual bool Initialzie(std::vector<DefaultVertex>& verts);
	virtual void Destroy();

protected:
	virtual bool CreateBuffer();
	virtual bool AllocateMemory();
	virtual bool UploadData(std::vector<DefaultVertex>& verts);

public:

	VkBuffer& GetBuffer() { return m_buffer; }
	VkDeviceMemory& GetMemory() { return m_memory; }
	VkDescriptorBufferInfo& GetBufferInfo() { return m_bufferInfo; }
	std::vector<VkVertexInputAttributeDescription>& GetVertexInputAttributeDescs() { return m_vertexAttrDescs; }
	VkVertexInputBindingDescription& GetBindingDescription() { return m_vertexBindingDesc; }

	uint32_t GetVertexCount() { return m_vertexCount; }
	uint32_t GetByteSize() { return m_byteSize; }

	uint32_t GetStride() { return m_stride; }

	VkFormat GetVertexFormat() { return m_vertexBufferFromat; }

protected:
	VkBuffer						m_buffer = VK_NULL_HANDLE;
	VkDeviceMemory					m_memory = VK_NULL_HANDLE;
	VkDescriptorBufferInfo			m_bufferInfo = {};
	VkVertexInputBindingDescription m_vertexBindingDesc = {};

	std::vector<VkVertexInputAttributeDescription>	m_vertexAttrDescs;

	VkFormat m_vertexBufferFromat = VK_FORMAT_R32G32B32_SFLOAT;

	uint32_t m_stride = 0;
	uint32_t m_vertexCount = 0;
	uint32_t m_byteSize = 0;
	VkDeviceAddress m_deviceAddress = 0;
};

class AsVertexBuffer : public VertexBuffer
{
public:
	virtual bool Initialzie(std::vector<DefaultVertex>& verts);
	virtual void Destroy() override;
protected:
	virtual bool CreateBuffer();
	virtual bool AllocateMemory();
	virtual bool UploadData(std::vector<DefaultVertex>& verts) override;
public:
	VkDeviceAddress GetDeviceAddress() { return m_deviceAddress; }
protected:
	VkBuffer		m_stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory	m_stagingMemory = VK_NULL_HANDLE;

	VkDeviceAddress m_deviceAddress = 0;
};

class IndexBuffer
{
public:
	IndexBuffer() {};
	~IndexBuffer() {};

public:
	virtual bool Initialzie(std::vector<uint32_t>& indices);
	virtual void Destroy();

protected:
	virtual bool CreateBuffer();
	virtual bool AllocateMemory();
	virtual bool UploadData(std::vector<uint32_t>& indices);

public:
	VkBuffer& GetBuffer() { return m_buffer; }
	VkDeviceMemory& GetMemory() { return m_memory; }
	VkDescriptorBufferInfo& GetBufferInfo() { return m_bufferInfo; }

	VkIndexType GetIndexType() { return m_indexType; }
	
	uint32_t GetIndexCount() { return m_indexCount; }
	uint32_t GetByteSize() { return m_byteSize; }

protected:

	VkBuffer				m_buffer = VK_NULL_HANDLE;
	VkDeviceMemory			m_memory = VK_NULL_HANDLE;
	VkDescriptorBufferInfo	m_bufferInfo = {};
	VkIndexType				m_indexType = VkIndexType::VK_INDEX_TYPE_UINT32;

	uint32_t m_indexCount = 0;
	uint32_t m_byteSize = 0;
};

class AsIndexBuffer : public IndexBuffer
{
public:
	virtual bool Initialzie(std::vector<uint32_t>& indices);
	virtual void Destroy() override;

protected:
	virtual bool CreateBuffer() override;
	virtual bool AllocateMemory() override;
	virtual bool UploadData(std::vector<uint32_t>& indices) override;
public:
	VkDeviceAddress GetDeviceAddress() { return m_deviceAddress; }
protected:
	
	VkBuffer		m_stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory	m_stagingMemory = VK_NULL_HANDLE;

	VkDeviceAddress m_deviceAddress = 0;
};

class RayTracingScratchBuffer : public BufferData
{
public:
	virtual bool Initialize(uint32_t size, VkBufferUsageFlags bufferUsage, VkFlags memRequirementsMask) override;
	VkDeviceAddress GetDeviceMemoryAddress() { return m_memoryAddress; }
private:
	VkDeviceAddress	m_memoryAddress = 0;
};