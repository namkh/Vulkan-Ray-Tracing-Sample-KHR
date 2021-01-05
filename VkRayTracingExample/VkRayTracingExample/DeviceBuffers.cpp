
#include "DeviceBuffers.h"
#include "Utils.h"

bool BufferData::Initialize(uint32_t size, VkBufferUsageFlags bufferUsage, VkFlags memRequirementsMask)
{
	if (m_isAllocated)
	{
		REPORT(EReportType::REPORT_TYPE_WARN, "BufferData is already allocated.");
		return false;
	}

	m_bufferUsage = bufferUsage;
	m_memRequirementsMask = memRequirementsMask;
	m_size = size;

	if (!CreateBuffer() || !AllocateMemory())
	{
		return false;
	}
	
	VkResult res = vkBindBufferMemory(gLogicalDevice,
		m_buffer,
		m_memory,
		0);
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "BufferData memory bind failed");
		return false;
	}

	m_bufferInfo.buffer = m_buffer;
	m_bufferInfo.offset = 0;
	m_bufferInfo.range = m_size;

	if ((m_bufferUsage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != 0)
	{
		m_memoryAddress = GetBufferDeviceAddress(m_buffer);
	}

	m_isAllocated = true;

	return true;
}

bool BufferData::Reset(uint32_t size, VkBufferUsageFlags bufferUsage, VkFlags memRequirementsMask)
{
	Destroy();
	if (!Initialize(m_size, bufferUsage, memRequirementsMask))
	{
		return false;
	}
	return true;
}

bool BufferData::Resize(uint32_t size)
{
	return Reset(size, m_bufferUsage, m_memRequirementsMask);
}

bool BufferData::UpdateResource(uint8_t* srcData, uint32_t offset, uint32_t size)
{
	if (offset + size > m_size)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "The requested size is larger than the buffer size.");
		return false;
	}

	uint8_t* data = nullptr;
	VkResult res = vkMapMemory(gLogicalDevice, m_memory, 0, m_size, 0, (void**)&data);
	if (res == VkResult::VK_SUCCESS)
	{
		data += offset;
		memcpy(data, srcData, size);

		vkUnmapMemory(gLogicalDevice, m_memory);
	}
	else
	{
		REPORT(EReportType::REPORT_TYPE_WARN, "Memory map failed.");
		return false;
	}
	return true;
}

void BufferData::Destroy()
{
	if (m_buffer != nullptr)
	{
		vkDestroyBuffer(gLogicalDevice, m_buffer, nullptr);
		m_buffer = VK_NULL_HANDLE;
	}
	if (m_memory != nullptr)
	{
		vkFreeMemory(gLogicalDevice, m_memory, nullptr);
		m_memory = VK_NULL_HANDLE;
	}
	m_isAllocated = false;
}

bool BufferData::CreateBuffer()
{
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = nullptr;
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.size = m_size;
	bufferCreateInfo.usage = m_bufferUsage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult res = vkCreateBuffer(gLogicalDevice, &bufferCreateInfo, nullptr, &m_buffer);
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "VkBuffer create failed.");
		return false;
	}
	return true;
}

bool BufferData::AllocateMemory()
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

	VkResult res = vkAllocateMemory(gLogicalDevice, &allocInfo, nullptr, &m_memory);
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Device memory allocation failed");
		return false;
	}

	return true;
}

bool VertexBuffer::Initialzie(std::vector<DefaultVertex>& verts)
{
	m_vertexCount = static_cast<uint32_t>(verts.size());
	m_stride = sizeof(DefaultVertex);
	m_byteSize = m_vertexCount * m_stride;

	if (!CreateBuffer() || !AllocateMemory())
	{
		return false;
	}

	if (!UploadData(verts))
	{
		//데이터 업로드 실패
		return false;
	}

	m_bufferInfo.buffer = m_buffer;
	m_bufferInfo.offset = 0;
	m_bufferInfo.range = sizeof(DefaultVertex) * m_vertexCount;

	m_vertexBindingDesc.binding = 0;
	m_vertexBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	m_vertexBindingDesc.stride = sizeof(DefaultVertex);

	m_vertexAttrDescs.resize(5);
	//vert
	m_vertexAttrDescs[0].location = 0;
	m_vertexAttrDescs[0].binding = 0;
	m_vertexAttrDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	m_vertexAttrDescs[0].offset = 0;
	//normal
	m_vertexAttrDescs[1].location = 1;
	m_vertexAttrDescs[1].binding = 0;
	m_vertexAttrDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	m_vertexAttrDescs[1].offset = 12;
	//tangent
	m_vertexAttrDescs[2].location = 2;
	m_vertexAttrDescs[2].binding = 0;
	m_vertexAttrDescs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	m_vertexAttrDescs[2].offset = 24;
	//color
	m_vertexAttrDescs[3].location = 3;
	m_vertexAttrDescs[3].binding = 0;
	m_vertexAttrDescs[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	m_vertexAttrDescs[3].offset = 36;
	//uv
	m_vertexAttrDescs[4].location = 4;
	m_vertexAttrDescs[4].binding = 0;
	m_vertexAttrDescs[4].format = VK_FORMAT_R32G32_SFLOAT;
	m_vertexAttrDescs[4].offset = 52;

	return true;
}

bool VertexBuffer::CreateBuffer()
{
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = nullptr;
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferCreateInfo.size = sizeof(DefaultVertex) * m_vertexCount;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 0;
	bufferCreateInfo.pQueueFamilyIndices = nullptr;
	
	VkResult res = vkCreateBuffer(gLogicalDevice, &bufferCreateInfo, nullptr, &m_buffer);
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "VkBuffer create failed.(vkCreateBuffer)");
		return false;
	}
	return true;
}

bool VertexBuffer::AllocateMemory()
{
	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(gLogicalDevice, m_buffer, &memReqs);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = 0;

	if (!VulkanDeviceResources::Instance().MemoryTypeFromProperties(memReqs.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
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
		REPORT(EReportType::REPORT_TYPE_ERROR, "Buffer memory bind failed.");
		return false;
	}

	return true;
}

bool VertexBuffer::UploadData(std::vector<DefaultVertex>& verts)
{
	if (m_buffer == VK_NULL_HANDLE && m_memory == VK_NULL_HANDLE)
	{
		REPORT(EReportType::REPORT_TYPE_WARN, "Buffer is not ready.");
		return false;
	}
	uint8_t* data = nullptr;
	VkResult res = vkMapMemory(gLogicalDevice, m_memory, 0, m_byteSize, 0, (void**)&data);
	if (res == VkResult::VK_SUCCESS)
	{
		memcpy(data, verts.data(), m_byteSize);
		vkUnmapMemory(gLogicalDevice, m_memory);
	}
	else
	{
		REPORT(EReportType::REPORT_TYPE_WARN, "Buffer memory map failed.");
		return false;
	}
	return true;
}

void VertexBuffer::Destroy()
{
	if (m_buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(gLogicalDevice, m_buffer, nullptr);
	}
	if (m_memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(gLogicalDevice, m_memory, nullptr);
	}
}

bool AsVertexBuffer::Initialzie(std::vector<DefaultVertex>& verts)
{
	if (!VertexBuffer::Initialzie(verts))
	{
		return false;
	}
	m_deviceAddress = GetBufferDeviceAddress(m_buffer);
	return true;
}

void AsVertexBuffer::Destroy()
{
	VertexBuffer::Destroy();

	if (m_stagingBuffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(gLogicalDevice, m_stagingBuffer, nullptr);
	}

	if (m_stagingMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(gLogicalDevice, m_stagingMemory, nullptr);
	}
}

bool AsVertexBuffer::CreateBuffer()
{
	//구간을 나눌게 아니라 param으로 받을까?
	VkBufferCreateInfo stagingBuifferCreateInfo = {};
	stagingBuifferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBuifferCreateInfo.pNext = nullptr;
	stagingBuifferCreateInfo.flags = 0;
	stagingBuifferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingBuifferCreateInfo.size = sizeof(DefaultVertex) * m_vertexCount;
	stagingBuifferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBufferCreateInfo bufferCreateInfo = stagingBuifferCreateInfo;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	
	if (vkCreateBuffer(gLogicalDevice, &stagingBuifferCreateInfo, nullptr, &m_stagingBuffer) != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_WARN, "Staging buffer create failed.");
		return false;
	}

	if (vkCreateBuffer(gLogicalDevice, &bufferCreateInfo, nullptr, &m_buffer) != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_WARN, "Buffer create failed.");
		return false;
	}
	return true;
}

bool AsVertexBuffer::AllocateMemory()
{
	VkMemoryRequirements stagingMemReqs;
	VkMemoryRequirements deviceMemReqs;
	
	vkGetBufferMemoryRequirements(gLogicalDevice, m_stagingBuffer, &stagingMemReqs);
	vkGetBufferMemoryRequirements(gLogicalDevice, m_buffer, &deviceMemReqs);

	VkMemoryAllocateFlagsInfo allocateFlagsInfo = {};
	allocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	allocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

	VkMemoryAllocateInfo stagingMemAllocInfo = {};
	stagingMemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	stagingMemAllocInfo.pNext = &allocateFlagsInfo;
	stagingMemAllocInfo.allocationSize = stagingMemReqs.size;
	stagingMemAllocInfo.memoryTypeIndex = 0;

	VkMemoryAllocateInfo deviceMemAllocInfo = stagingMemAllocInfo;
	deviceMemAllocInfo.allocationSize = deviceMemReqs.size;

	if (!VulkanDeviceResources::Instance().MemoryTypeFromProperties(stagingMemReqs.memoryTypeBits,
																	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
																	VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
																	&stagingMemAllocInfo.memoryTypeIndex))
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Not found device meory type.");
		return false;
	}

	if (!VulkanDeviceResources::Instance().MemoryTypeFromProperties(deviceMemReqs.memoryTypeBits,
																	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
																	VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
																	&deviceMemAllocInfo.memoryTypeIndex))
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Not found device meory type.");
		return false;
	}

	if (vkAllocateMemory(gLogicalDevice, &stagingMemAllocInfo, nullptr, &m_stagingMemory) != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Device memory allocation failed.");
		return false;
	}

	if (vkAllocateMemory(gLogicalDevice, &deviceMemAllocInfo, nullptr, &m_memory) != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Device memory allocation failed.");
		return false;
	}

	if (vkBindBufferMemory(gLogicalDevice, m_stagingBuffer, m_stagingMemory, 0) != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Buffer bind failed.");
		return false;
	}

	if (vkBindBufferMemory(gLogicalDevice, m_buffer, m_memory, 0) != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Buffer bind failed.");
		return false;
	}

	return true;
}

bool AsVertexBuffer::UploadData(std::vector<DefaultVertex>& verts)
{
	if (m_buffer == VK_NULL_HANDLE || m_memory == VK_NULL_HANDLE || m_stagingBuffer == VK_NULL_HANDLE || m_stagingMemory == VK_NULL_HANDLE)
	{
		REPORT(EReportType::REPORT_TYPE_WARN, "Buffer is not ready.");
		return false;
	}
	uint8_t* data = nullptr;
	VkResult res = vkMapMemory(gLogicalDevice, m_stagingMemory, 0, m_byteSize, 0, (void**)&data);
	if (res == VkResult::VK_SUCCESS)
	{
		memcpy(data, verts.data(), m_byteSize);
		vkUnmapMemory(gLogicalDevice, m_stagingMemory);
	}
	else
	{
		REPORT(EReportType::REPORT_TYPE_WARN, "Buffer memory map failed");
		return false;
	}

	SingleTimeCommandBuffer singleTimeCmdBuf;
	singleTimeCmdBuf.Begin();
	VkBufferCopy bufferCopy = {};
	bufferCopy.size = m_byteSize;
	vkCmdCopyBuffer(singleTimeCmdBuf.GetCommandBuffer(), m_stagingBuffer, m_buffer, 1, &bufferCopy);
	singleTimeCmdBuf.End();

	return true;
}

bool IndexBuffer::Initialzie(std::vector<uint32_t>& indices)
{
	m_indexCount = static_cast<uint32_t>(indices.size());
	m_byteSize = m_indexCount * sizeof(uint32_t);

	if (!CreateBuffer() || !AllocateMemory())
	{
		return false;
	}

	if (!UploadData(indices))
	{
		return false;
	}

	m_bufferInfo.buffer = m_buffer;
	m_bufferInfo.offset = 0;
	m_bufferInfo.range = m_byteSize;
	
	return true;
}

bool IndexBuffer::CreateBuffer()
{
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = nullptr;
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	bufferCreateInfo.size = sizeof(uint32_t) * m_indexCount;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 0;
	bufferCreateInfo.pQueueFamilyIndices = nullptr;

	VkResult res = vkCreateBuffer(gLogicalDevice, &bufferCreateInfo, nullptr, &m_buffer);
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Buffer create failed");
		return false;
	}
	return true;
}

bool IndexBuffer::AllocateMemory()
{
	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(gLogicalDevice, m_buffer, &memReqs);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = 0;

	if (!VulkanDeviceResources::Instance().MemoryTypeFromProperties(memReqs.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&allocInfo.memoryTypeIndex))
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Not found device meory type.");
		return false;
	}

	VkResult res = vkAllocateMemory(gLogicalDevice, &allocInfo, nullptr, &m_memory);
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Device memory allocation failed.");
		return false;
	}

	res = vkBindBufferMemory(gLogicalDevice, m_buffer, m_memory, 0);
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Buffer bind failed.");
		return false;
	}

	return true;
}

bool IndexBuffer::UploadData(std::vector<uint32_t>& indices)
{
	if (m_buffer == VK_NULL_HANDLE || m_memory == VK_NULL_HANDLE)
	{
		REPORT(EReportType::REPORT_TYPE_WARN, "Buffer is not ready.");
		return false;
	}
	uint8_t* data = nullptr;
	VkResult res = vkMapMemory(gLogicalDevice, m_memory, 0, m_byteSize, 0, (void**)&data);
	if (res == VkResult::VK_SUCCESS)
	{
		memcpy(data, indices.data(), m_byteSize);

		vkUnmapMemory(gLogicalDevice, m_memory);
	}
	else
	{
		REPORT(EReportType::REPORT_TYPE_WARN, "Buffer memory map failed");
		return false;
	}
	return true;
}

void IndexBuffer::Destroy()
{
	if (m_buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(gLogicalDevice, m_buffer, nullptr);
	}
	if (m_memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(gLogicalDevice, m_memory, nullptr);
	}
}

bool AsIndexBuffer::Initialzie(std::vector<uint32_t>& indices)
{
	if (!IndexBuffer::Initialzie(indices))
	{
		return false;
	}
	m_deviceAddress = GetBufferDeviceAddress(m_buffer);
	return true;
}

void AsIndexBuffer::Destroy()
{
	IndexBuffer::Destroy();

	if (m_stagingBuffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(gLogicalDevice, m_stagingBuffer, nullptr);
	}
	if (m_stagingMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(gLogicalDevice, m_stagingMemory, nullptr);
	}
}

bool AsIndexBuffer::CreateBuffer()
{
	VkBufferCreateInfo stagingBufferCreateInfo = {};
	stagingBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferCreateInfo.pNext = nullptr;
	stagingBufferCreateInfo.flags = 0;
	stagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingBufferCreateInfo.size = sizeof(uint32_t) * m_indexCount;
	stagingBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(gLogicalDevice, &stagingBufferCreateInfo, nullptr, &m_stagingBuffer) != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Staging buffer create failed.");
		return false;
	}

	VkBufferCreateInfo deviceBufferCreateInfo = {};
	deviceBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	deviceBufferCreateInfo.pNext = nullptr;
	deviceBufferCreateInfo.flags = 0;
	deviceBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	deviceBufferCreateInfo.size = sizeof(uint32_t) * m_indexCount;
	deviceBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(gLogicalDevice, &deviceBufferCreateInfo, nullptr, &m_buffer) != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Buffer create failed.");
		return false;
	}
	return true;
}

bool AsIndexBuffer::AllocateMemory()
{
	VkMemoryRequirements stagingMemReqs;
	VkMemoryRequirements deviceMemReqs;
	vkGetBufferMemoryRequirements(gLogicalDevice, m_stagingBuffer, &stagingMemReqs);
	vkGetBufferMemoryRequirements(gLogicalDevice, m_buffer, &deviceMemReqs);

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

	if (!VulkanDeviceResources::Instance().MemoryTypeFromProperties(stagingMemReqs.memoryTypeBits,
																	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
																	VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
																	&stagingMemAllocInfo.memoryTypeIndex))
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Not found device meory type.");
		return false;
	}

	if (!VulkanDeviceResources::Instance().MemoryTypeFromProperties(deviceMemReqs.memoryTypeBits,
																	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
																	&deviceMemAllocInfo.memoryTypeIndex))
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Not found device meory type.");
		return false;
	}

	VkResult res = vkAllocateMemory(gLogicalDevice, &stagingMemAllocInfo, nullptr, &m_stagingMemory);
	if (res != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Device memory allocation failed.");
		return false;
	}

	if (vkAllocateMemory(gLogicalDevice, &deviceMemAllocInfo, nullptr, &m_memory) != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Device memory allocation failed.");
		return false;
	}

	if (vkBindBufferMemory(gLogicalDevice, m_stagingBuffer, m_stagingMemory, 0) != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Buffer bind failed.");
		return false;
	}

	if (vkBindBufferMemory(gLogicalDevice, m_buffer, m_memory, 0) != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Buffer bind failed.");
		return false;
	}

	return true;
}

bool AsIndexBuffer::UploadData(std::vector<uint32_t>& indices)
{
	if (m_buffer == VK_NULL_HANDLE || m_stagingBuffer == VK_NULL_HANDLE || m_memory == VK_NULL_HANDLE || m_stagingMemory == VK_NULL_HANDLE)
	{
		REPORT(EReportType::REPORT_TYPE_WARN, "Buffer is not ready.");
		return false;
	}
	uint8_t* data = nullptr;
	VkResult res = vkMapMemory(gLogicalDevice, m_stagingMemory, 0, m_byteSize, 0, (void**)&data);
	if (res == VkResult::VK_SUCCESS)
	{
		memcpy(data, indices.data(), m_byteSize);

		vkUnmapMemory(gLogicalDevice, m_stagingMemory);
	}
	else
	{
		REPORT(EReportType::REPORT_TYPE_WARN, "Buffer memory map failed");
		return false;
	}

	SingleTimeCommandBuffer singleTimeCmdBuf;
	singleTimeCmdBuf.Begin();
	VkBufferCopy bufferCopy = {};
	bufferCopy.size = m_byteSize;
	vkCmdCopyBuffer(singleTimeCmdBuf.GetCommandBuffer(), m_stagingBuffer, m_buffer, 1, &bufferCopy);
	singleTimeCmdBuf.End();

	return true;
}

bool RayTracingScratchBuffer::Initialize(uint32_t size, VkBufferUsageFlags bufferUsage, VkFlags memRequirementsMask)
{
	if (BufferData::Initialize(size, bufferUsage, memRequirementsMask))
	{
		m_memoryAddress = GetBufferDeviceAddress(m_buffer);
		return true;
	}
	return false;
}

/*
bool RayTracingScratchBuffer::Initialize(VkAccelerationStructureKHR as)
{
	if (m_isAllocated)
	{
		REPORT(EReportType::REPORT_TYPE_WARN, "BufferData is already allocated.");
		return false;
	}

	VkMemoryRequirements2 memReq2 = {};
	memReq2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

	VkAccelerationStructureMemoryRequirementsInfoKHR asMemReqInfo = {};
	asMemReqInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR;
	asMemReqInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR;
	asMemReqInfo.buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;
	asMemReqInfo.accelerationStructure = as;
	vkGetAccelerationStructureMemoryRequirementsKHR(gLogicalDevice, &asMemReqInfo, &memReq2);

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = memReq2.memoryRequirements.size;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(gLogicalDevice, &bufferCreateInfo, nullptr, &m_buffer) != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Buffer create failed.");
		return false;
	}

	VkMemoryRequirements bufferMemReq = {};
	vkGetBufferMemoryRequirements(gLogicalDevice, m_buffer, &bufferMemReq);

	uint32_t memTypeIndex = 0;
	if (VulkanDeviceResources::Instance().MemoryTypeFromProperties(bufferMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memTypeIndex))
	{
		VkMemoryAllocateFlagsInfo memAllocFlagsInfo = {};
		memAllocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
		memAllocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

		VkMemoryAllocateInfo memAllocInfo = {};
		memAllocInfo.sType				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAllocInfo.pNext				= &memAllocFlagsInfo;
		memAllocInfo.allocationSize		= bufferMemReq.size;
		memAllocInfo.memoryTypeIndex	= memTypeIndex;

		if (vkAllocateMemory(gLogicalDevice, &memAllocInfo, nullptr, &m_memory) != VkResult::VK_SUCCESS)
		{
			REPORT(EReportType::REPORT_TYPE_ERROR, "Device memory allocation failed.");
			return false;
		}
		if (vkBindBufferMemory(gLogicalDevice, m_buffer, m_memory, 0) != VkResult::VK_SUCCESS)
		{
			REPORT(EReportType::REPORT_TYPE_ERROR, "Buffer bind failed.");
			return false;
		}

		m_memoryAddress = GetBufferDeviceAddress(m_buffer);
	}
	else
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Not found device meory type.");
		return false;
	}

	m_isAllocated = true;

	return true;
}

void RayTracingScratchBuffer::Destroy()
{
	if (m_memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(gLogicalDevice, m_memory, nullptr);
		m_memory = VK_NULL_HANDLE;
	}
	if (m_buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(gLogicalDevice, m_buffer, nullptr);
		m_buffer = VK_NULL_HANDLE;
	}
	m_isAllocated = false;
}
*/
/*
bool RayTracingAccelerationStructureMemory::Initialize(VkAccelerationStructureKHR as)
{
	if (m_isAllocated)
	{
		REPORT(EReportType::REPORT_TYPE_WARN, "BufferData is already allocated.");
		return false;
	}
	VkMemoryRequirements2 memReq2 = {};
	memReq2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

	VkAccelerationStructureMemoryRequirementsInfoKHR asMemReqInfo = {};
	asMemReqInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR;
	asMemReqInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_KHR;
	asMemReqInfo.buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;
	asMemReqInfo.accelerationStructure = as;
	vkGetAccelerationStructureMemoryRequirementsKHR(gLogicalDevice, &asMemReqInfo, &memReq2);

	uint32_t memTypeIndex = 0;
	if (VulkanDeviceResources::Instance().MemoryTypeFromProperties(memReq2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memTypeIndex))
	{
		VkMemoryAllocateInfo memAllocInfo = {};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAllocInfo.allocationSize = memReq2.memoryRequirements.size;
		memAllocInfo.memoryTypeIndex = memTypeIndex;
		if (vkAllocateMemory(gLogicalDevice, &memAllocInfo, nullptr, &m_memory) != VkResult::VK_SUCCESS)
		{
			REPORT(EReportType::REPORT_TYPE_ERROR, "Device memory allocation failed.");
			return false;
		}
	}
	else
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Not found device meory type.");
		return false;
	}


	VkBindAccelerationStructureMemoryInfoKHR asMemBindInfo = {};
	asMemBindInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_KHR;
	asMemBindInfo.accelerationStructure = as;
	asMemBindInfo.memory = m_memory;
	
	if (vkBindAccelerationStructureMemoryKHR(gLogicalDevice, 1, &asMemBindInfo) != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Buffer bind failed.");
		return false;
	}

	m_isAllocated = true;

	return true;
}

void RayTracingAccelerationStructureMemory::Destroy()
{
	if (m_memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(gLogicalDevice, m_memory, nullptr);
		m_memory= VK_NULL_HANDLE;
	}
	m_isAllocated = false;
}

*/