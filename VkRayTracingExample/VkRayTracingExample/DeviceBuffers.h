#pragma once

#include <stdlib.h>

#include "VulkanDeviceResources.h"
#include "CommandBuffers.h"

class BufferData
{
public:
	virtual bool Initialzie(uint32_t size, VkBufferUsageFlags bufferUsage, VkFlags memRequirementsMask);
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

	uint32_t GetBufferSize() { return m_size; }
	bool IsAllocated() { return m_isAllocated; }

public:
	VkBuffer				m_buffer = nullptr;
	VkBufferUsageFlags      m_bufferUsage = 0;
	VkFlags					m_memRequirementsMask = 0;
	VkDeviceMemory			m_memory = nullptr;

	VkDescriptorBufferInfo	m_bufferInfo = {};
	

	uint32_t m_size = 0;
	bool m_isAllocated = false;
};


//데이터 버퍼들을 베이스를 갖고 추상화를 했을때 좋은점이 있을까?
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

	bool IsAllocated() { return m_isAllocated; }

protected:
	VkBuffer				m_buffer = VK_NULL_HANDLE;
	VkDeviceMemory			m_memory = VK_NULL_HANDLE;
	VkDescriptorBufferInfo	m_bufferInfo = {};
	VkBufferUsageFlags		m_bufferUsage = 0;
	VkFlags					m_memRequirementsMask = 0;

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
		//자원이 이미 할당 되어있음을 로깅
		return false;
	}
	m_numDatas = numDatas;
	m_stride = sizeof(ResourceType);
	m_byteSize = m_stride * m_numDatas;
	m_bufferUsage = bufferUsage;
	m_memRequirementsMask = memRequirementsMask;

	if (m_numDatas < 1)
	{
		//데이터 카운트는 1이상이어야함
		return false;
	}
	
	if (!CreateBuffer() || !AllocateMemory())
	{
		return false;
	}

	m_bufferInfo.buffer = m_buffer;
	m_bufferInfo.offset = 0;
	m_bufferInfo.range = m_byteSize;

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
		//버퍼 할당 실패를 로깅
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
		//버퍼 생성 실패에 대한 로깅
		return false;
	}
	return true;
}

template <typename ResourceType>
bool StructuredBufferData<ResourceType>::AllocateMemory()
{
	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(gLogicalDevice, m_buffer, &memReqs);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = 0;

	if (!VulkanDeviceResources::Instance().MemoryTypeFromProperties(memReqs.memoryTypeBits,
		m_memRequirementsMask,
		&allocInfo.memoryTypeIndex))
	{
		//메모리 타입 없음을 로깅
		return false;
	}

	if (vkAllocateMemory(gLogicalDevice, &allocInfo, nullptr, &m_memory) != VkResult::VK_SUCCESS)
	{
		//디바이스 메모리 할당실패를 로깅
		return false;
	}

	if (vkBindBufferMemory(gLogicalDevice, m_buffer, m_memory, 0) != VkResult::VK_SUCCESS)
	{
		//버퍼 바인드 실패로깅
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
			//메모리 맵 실패 로그
		}
	}
	else
	{
		//데이터가 사이즈가 맞지 않음을 로깅
	}
}

template <typename ResourceType>
void StructuredBufferData<ResourceType>::UpdateResource(ResourceType& srcData, uint32_t dataIndex)
{
	if (dataIndex >= m_numDatas || dataIndex < 0)
	{
		//데이터 업데이트 실패 로깅
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
		//메모리 맵 실패 로그
	}
}

template <typename ResourceType>
void StructuredBufferData<ResourceType>::UpdateResource(ResourceType& srcData, std::vector<uint32_t>& updateIndices)
{
	for (int i = 0; i < updateIndices.size(); i++)
	{
		if (updateIndices[i] >= m_numDatas)
		{
			//자원 범위를 넘어서는 인덱스를 가졌음을 로깅
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
		//메모리 맵 실패 로그
	}
	vkUnmapMemory(gLogicalDevice, m_memory);

}

template <typename ResourceType>
class RtBufferData : public StructuredBufferData<ResourceType>
{
public:
	virtual bool Initialzie(VkBufferUsageFlags bufferUsage, VkFlags memRequirementsMask, uint32_t numDatas = 1) override;
	virtual void Destroy() override;
protected:
	virtual bool CreateBuffer();
	virtual bool AllocateMemory();

public:
	virtual void UpdateResource(ResourceType* srcData, uint32_t bufferCount) override;
	virtual void UpdateResource(ResourceType& srcData, uint32_t index = 0) override;

public:
	VkDeviceAddress	GetDeviceAddress() { return m_deviceAddress; }

protected:
	VkBuffer m_stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory m_stagingMemory = VK_NULL_HANDLE;

	VkDeviceAddress m_deviceAddress = 0;
};

template <typename ResourceType>
bool RtBufferData<ResourceType>::Initialzie(VkBufferUsageFlags bufferUsage, VkFlags memRequirementsMask, uint32_t numDatas)
{
	if (!StructuredBufferData<ResourceType>::Initialzie(bufferUsage, memRequirementsMask, numDatas))
	{
		return false;
	}
	m_deviceAddress = GetBufferDeviceAddress(StructuredBufferData<ResourceType>::GetBuffer());
	return true;
}

template <typename ResourceType>
void RtBufferData<ResourceType>::Destroy()
{
	StructuredBufferData<ResourceType>::Destroy();

	if (m_stagingBuffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(gLogicalDevice, m_stagingBuffer, nullptr);
	}
	if (m_stagingMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(gLogicalDevice, m_stagingMemory, nullptr);
	}
}

template <typename ResourceType>
bool RtBufferData<ResourceType>::CreateBuffer()
{
	VkBufferCreateInfo stagingBufferCreateInfo = {};
	stagingBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferCreateInfo.pNext = nullptr;
	stagingBufferCreateInfo.flags = 0;
	stagingBufferCreateInfo.size = static_cast<VkDeviceSize>(StructuredBufferData<ResourceType>::GetStride()) * static_cast<VkDeviceSize>(StructuredBufferData<ResourceType>::GetNumDatas());
	stagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBufferCreateInfo deviceBufferCreateInfo = stagingBufferCreateInfo;
	deviceBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	
	VkResult res = vkCreateBuffer(gLogicalDevice, &stagingBufferCreateInfo, nullptr, &m_stagingBuffer);
	if (res != VkResult::VK_SUCCESS)
	{
		//버퍼 생성 실패에 대한 로깅
		return false;
	}

	res = vkCreateBuffer(gLogicalDevice, &deviceBufferCreateInfo, nullptr, &StructuredBufferData<ResourceType>::GetBuffer());
	if (res != VkResult::VK_SUCCESS)
	{
		//버퍼 생성 실패에 대한 로깅
		return false;
	}
	return true;
}

template <typename ResourceType>
bool RtBufferData<ResourceType>::AllocateMemory()
{
	VkMemoryRequirements stagingMemReqs;
	VkMemoryRequirements deviceMemReqs;
	vkGetBufferMemoryRequirements(gLogicalDevice, m_stagingBuffer, &stagingMemReqs);
	vkGetBufferMemoryRequirements(gLogicalDevice, StructuredBufferData<ResourceType>::GetBuffer(), &deviceMemReqs);

	VkMemoryAllocateFlagsInfo memAllocateFlagsInfo = {};
	memAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	memAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

	VkMemoryAllocateInfo stagingMemAllocInfo = {};
	stagingMemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	stagingMemAllocInfo.pNext = &memAllocateFlagsInfo;
	stagingMemAllocInfo.allocationSize = stagingMemReqs.size;
	stagingMemAllocInfo.memoryTypeIndex = 0;

	VkMemoryAllocateInfo deviceMemAllocInfo = stagingMemAllocInfo;
	deviceMemAllocInfo.allocationSize = deviceMemReqs.size;

	//메모리 프로퍼티에서 할당 메모리 타입을 찾는다
	if (!VulkanDeviceResources::Instance().MemoryTypeFromProperties(stagingMemReqs.memoryTypeBits,
																	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
																	VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
																	&stagingMemAllocInfo.memoryTypeIndex))
	{
		//메모리 타입 없음을 로깅
		return false;
	}

	if (!VulkanDeviceResources::Instance().MemoryTypeFromProperties(deviceMemReqs.memoryTypeBits,
																	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
																	&deviceMemAllocInfo.memoryTypeIndex))
	{
		//메모리 타입 없음을 로깅
		return false;
	}

	if (vkAllocateMemory(gLogicalDevice, &stagingMemAllocInfo, nullptr, &m_stagingMemory))
	{
		//스테이징 메모리 할당실패를 로깅
		return false;
	}

	if (vkAllocateMemory(gLogicalDevice, &deviceMemAllocInfo, nullptr, &StructuredBufferData<ResourceType>::GetMemory()))
	{
		//디바이스 메모리 할당실패를 로깅
		return false;
	}

	if (vkBindBufferMemory(gLogicalDevice, m_stagingBuffer, m_stagingMemory, 0) != VkResult::VK_SUCCESS)
	{
		//스테이징 버퍼 바인드 실패로깅
		return false;
	}

	if (vkBindBufferMemory(gLogicalDevice, StructuredBufferData<ResourceType>::GetBuffer(), StructuredBufferData<ResourceType>::GetMemory(), 0) != VkResult::VK_SUCCESS)
	{
		//버퍼 바인드 실패로깅
		return false;
	}

	return true;
}

template <typename ResourceType>
void RtBufferData<ResourceType>::UpdateResource(ResourceType* srcData, uint32_t bufferCount)
{
	if (bufferCount == StructuredBufferData<ResourceType>::GetNumDatas())
	{
		uint8_t* data = nullptr;
		VkResult res = vkMapMemory(gLogicalDevice, m_stagingMemory, 0, sizeof(ResourceType), 0, (void**)&data);
		if (res == VkResult::VK_SUCCESS)
		{
			memcpy(data, srcData, StructuredBufferData<ResourceType>::GetByteSize());
			vkUnmapMemory(gLogicalDevice, m_stagingMemory);
		}
		else
		{
			//메모리 맵 실패 로그
		}

		SingleTimeCommandBuffer singleTimeCmdBuf;
		singleTimeCmdBuf.Begin();
		VkBufferCopy bufferCopy = {};
		bufferCopy.size = StructuredBufferData<ResourceType>::GetByteSize();
		vkCmdCopyBuffer(singleTimeCmdBuf.GetCommandBuffer(), m_stagingBuffer, StructuredBufferData<ResourceType>::GetBuffer(), 1, &bufferCopy);
		singleTimeCmdBuf.End();
	}
	else
	{
		//데이터가 사이즈가 맞지 않음을 로깅
	}
}

template <typename ResourceType>
void RtBufferData<ResourceType>::UpdateResource(ResourceType& srcData, uint32_t index)
{
	if (index >= StructuredBufferData<ResourceType>::GetNumDatas() || index < 0)
	{
		//데이터 업데이트 실패 로깅
		return;
	}

	uint8_t* data = nullptr;
	VkResult res = vkMapMemory(gLogicalDevice, m_stagingMemory, 0, sizeof(ResourceType), 0, (void**)&data);
	if (res == VkResult::VK_SUCCESS)
	{
		data += sizeof(ResourceType) * index;
		memcpy(data, &srcData, sizeof(ResourceType));

		vkUnmapMemory(gLogicalDevice, m_stagingMemory);
	}
	else
	{
		//메모리 맵 실패 로그
	}

	SingleTimeCommandBuffer singleTimeCmdBuf;
	singleTimeCmdBuf.Begin();
	VkBufferCopy bufferCopy = {};
	bufferCopy.size = StructuredBufferData<ResourceType>::GetByteSize();
	vkCmdCopyBuffer(singleTimeCmdBuf.GetCommandBuffer(), m_stagingBuffer, StructuredBufferData<ResourceType>::GetBuffer(), 1, &bufferCopy);
	singleTimeCmdBuf.End();
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

class RayTracingScratchBuffer
{
public:
	bool Initialize(VkAccelerationStructureKHR as);
	void Destroy();

	VkBuffer GetBuffer() { return m_buffer; }
	VkDeviceMemory GetDeviceMemory() { return m_memory; }
	VkDeviceAddress GetDeviceMemoryAddress() { return m_memoryAddress; }

	bool IsAllocated() { return m_isAllocated; }

private:
	VkBuffer		m_buffer				= VK_NULL_HANDLE;
	VkDeviceMemory	m_memory				= VK_NULL_HANDLE;
	VkDeviceAddress	m_memoryAddress			= 0;

	bool m_isAllocated = false;
};

class RayTracingAccelerationStructureMemory
{
public:
	bool Initialize(VkAccelerationStructureKHR as);
	void Destroy();

	VkDeviceMemory GetDeviceMemory() { return m_memory; }
	uint64_t GetDeviceMemoryAddress() { return m_memoryAddress; }

	bool IsAllocated() { return m_isAllocated; }
	
private:
	VkDeviceMemory m_memory = VK_NULL_HANDLE;
	uint64_t m_memoryAddress = 0;

	bool m_isAllocated = false;

};