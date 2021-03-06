
#include "RTAccelerationStructure.h"
#include "RenderObjectContainer.h"
#include "ShaderContainer.h"

void RayTracingAccelerationStructureBase::Destroy()
{
	m_asMemory.Destroy();
	m_scratchBuffer.Destroy();
	if (m_accelerationStructure != VK_NULL_HANDLE)
	{
		vkDestroyAccelerationStructureKHR(gLogicalDevice, m_accelerationStructure, nullptr);
	}
}

void RayTracingAccelerationStructureBase::Reset()
{
	Destroy();
	m_transformMatrix = {};
	m_handle = 0;
	m_isBuilded = false;
}

bool BottomLevelAS::Build(VkCommandBuffer commandBuffer, bool update)
{
	if (m_sourceMesh == nullptr)
	{
		return false;
	}

	if (!update)
	{
		VkDeviceOrHostAddressConstKHR vertexDataDeviceAddress = {};
		VkDeviceOrHostAddressConstKHR indexDataDeviceAddress = {};

		AsVertexBuffer* vertexBuffer = m_sourceMesh->GetVertexBuffer();
		AsIndexBuffer* indexBuffer = m_sourceMesh->GetIndexBuffer();
		if (vertexBuffer == nullptr || indexBuffer == nullptr)
		{
			return false;
		}

		vertexDataDeviceAddress.deviceAddress = GetBufferDeviceAddress(vertexBuffer->GetBuffer());
		indexDataDeviceAddress.deviceAddress = GetBufferDeviceAddress(indexBuffer->GetBuffer());

		m_primitiveCount = indexBuffer->GetIndexCount() / 3;

		m_bottomLevelAsGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		m_bottomLevelAsGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		m_bottomLevelAsGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		m_bottomLevelAsGeometry.geometry.triangles.vertexFormat = vertexBuffer->GetVertexFormat();
		m_bottomLevelAsGeometry.geometry.triangles.vertexData.deviceAddress = vertexBuffer->GetDeviceAddress();
		m_bottomLevelAsGeometry.geometry.triangles.vertexStride = vertexBuffer->GetStride();
		m_bottomLevelAsGeometry.geometry.triangles.maxVertex = vertexBuffer->GetVertexCount();
		m_bottomLevelAsGeometry.geometry.triangles.indexType = indexBuffer->GetIndexType();
		m_bottomLevelAsGeometry.geometry.triangles.indexData.deviceAddress = indexBuffer->GetDeviceAddress();
		m_bottomLevelAsGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

		VkAccelerationStructureBuildGeometryInfoKHR asBuildGeomInfo{};
		asBuildGeomInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		asBuildGeomInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		asBuildGeomInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		asBuildGeomInfo.geometryCount = 1;
		asBuildGeomInfo.pGeometries = &m_bottomLevelAsGeometry;

		VkAccelerationStructureBuildSizesInfoKHR asBuildSizeInfo{};
		asBuildSizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		vkGetAccelerationStructureBuildSizesKHR
		(
			gLogicalDevice,
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&asBuildGeomInfo,
			&m_primitiveCount,
			&asBuildSizeInfo
		);

		if (!m_asMemory.Initialize(static_cast<uint32_t>(asBuildSizeInfo.accelerationStructureSize), VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
		{
			return false;
		}

		VkAccelerationStructureCreateInfoKHR asCreateInfo{};
		asCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		asCreateInfo.buffer = m_asMemory.GetBuffer();
		asCreateInfo.size = asBuildSizeInfo.accelerationStructureSize;
		asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

		if (vkCreateAccelerationStructureKHR(gLogicalDevice, &asCreateInfo, nullptr, &m_accelerationStructure) != VkResult::VK_SUCCESS)
		{
			REPORT(EReportType::REPORT_TYPE_ERROR, "Blas create failed.");
			return false;
		}

		if (!m_scratchBuffer.Initialize(static_cast<uint32_t>(asBuildSizeInfo.buildScratchSize), VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
		{
			return false;
		}
	}

	VkAccelerationStructureBuildGeometryInfoKHR asBuildGeomInfo = {};
	asBuildGeomInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	asBuildGeomInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	asBuildGeomInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
	if (update)
	{	
		asBuildGeomInfo.srcAccelerationStructure = m_accelerationStructure;
	}
	asBuildGeomInfo.dstAccelerationStructure = m_accelerationStructure;
	asBuildGeomInfo.geometryCount = 1;
	asBuildGeomInfo.pGeometries = &m_bottomLevelAsGeometry;
	asBuildGeomInfo.scratchData.deviceAddress = m_scratchBuffer.GetDeviceMemoryAddress();

	VkAccelerationStructureBuildRangeInfoKHR asBuildRangeInfo = {};
	asBuildRangeInfo.primitiveCount = m_primitiveCount;
	asBuildRangeInfo.primitiveOffset = 0;
	asBuildRangeInfo.firstVertex = 0;
	asBuildRangeInfo.transformOffset = 0;

	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> asBuildRangeInfos = { &asBuildRangeInfo };

	vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &asBuildGeomInfo, asBuildRangeInfos.data());

	VkMemoryBarrier memoryBarrier;
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.pNext = nullptr;
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
	memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
	vkCmdPipelineBarrier
	(
		commandBuffer, 
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 
		0, 
		1, 
		&memoryBarrier,
		0, 
		nullptr, 
		0, 
		nullptr
	);

	VkAccelerationStructureDeviceAddressInfoKHR asDeviceAddressInfo = {};
	asDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	asDeviceAddressInfo.accelerationStructure = m_accelerationStructure;
	m_handle = vkGetAccelerationStructureDeviceAddressKHR(gLogicalDevice, &asDeviceAddressInfo);

	m_buildState = EBlasBuildState::BUILDED;

	return true;
}


void BottomLevelAsGroup::Clear()
{
	for (auto& cur : m_blasList)
	{
		if (cur != nullptr)
		{
			cur->Destroy();
			delete cur;
		}
	}
	m_instancesDeviceBuffer.Destroy();
}

void BottomLevelAsGroup::Build(VkCommandBuffer commandBuffer)
{
	if (!m_isBuilded)
	{
		uint32_t meshCount = gGeomContainer.GetMeshCount();
		m_blasList.resize(meshCount);
		for (uint32_t i = 0; i < meshCount; i++)
		{
			SimpleMeshData* mesh = gGeomContainer.GetMesh(i);
			m_blasList[i] = new BottomLevelAS();
			m_blasList[i]->SetSourceMesh(mesh);
			m_blasList[i]->Build(commandBuffer);
		}

		RefreshBlasList();

		m_meshLoadedCallbackHandle = gGeomContainer.OnMeshLoaded.Add
		(
			[this](UID uid)
			{
				OnMeshAdded(uid);
			}
		);

		m_meshUnloadedCallbackHandle = gGeomContainer.OnMeshUnloaded.Add
		(
			[this](UID uid)
			{
				OnMeshRemoved(uid);
			}
		);

		m_instPreMeshAddedCallbackHandle = gRenderObjContainer.OnInstPerMeshAdded.Add
		(
			[this](uint32_t index)
			{
				OnInstPerMeshAdded(index);
			}
		);

		m_instPerMeshRemovedCallbackHandle = gRenderObjContainer.OnInstPerMeshRemoved.Add
		(
			[this](uint32_t index)
			{
				OnInstPerMeshRemoved(index);
			}
		);
	}
}

void BottomLevelAsGroup::Update(VkCommandBuffer commandBuffer)
{
	if (commandBuffer != VK_NULL_HANDLE)
	{
		for (auto& cur : m_blasList)
		{
			if (cur->GetBuildState() != EBlasBuildState::BUILDED)
			{
				cur->Build(commandBuffer, true);
			}
		}
	}

	if (m_instanceListChanged)
	{
		RefreshInstanceBufferDatas();
		RefreshBlasList(true);
	}
}

void BottomLevelAsGroup::SetInstanceData(int index, bool update)
{
	SampleRenderObjectInstancePerMesh* curInstPerMesh = gRenderObjContainer.GetRenderObjectInstancePerMesh(index);
	if (!update)
	{
		curInstPerMesh->GetParentInstance()->OnInstanceUpdated.Add
		(
			[this](uint32_t index)
			{
				OnInstPerMeshUpdated(index);
			}
		);
		
		UID meshUID = curInstPerMesh->GetMeshData()->GetUID();
		curInstPerMesh->OnMeshUpdated.Add
		(
			[this, meshUID](VkCommandBuffer cmdBuffer)
			{
				OnMeshUpdated(meshUID);
			}
		);
	}

	glm::mat3x4 curMat = glm::mat3x4(glm::transpose(curInstPerMesh->GetParentInstance()->GetWorldMatrix()));
	m_asInstances[index].transform = 
	{
		curMat[0].x, curMat[0].y, curMat[0].z, curMat[0].w,
		curMat[1].x, curMat[1].y, curMat[1].z, curMat[1].w,
		curMat[2].x, curMat[2].y, curMat[2].z, curMat[2].w
	};

	SimpleMaterial* material = curInstPerMesh->GetMaterial();

	uint32_t meshIndex = gGeomContainer.GetMeshBindIndex(curInstPerMesh->GetMeshData());
	uint32_t hitGroupIndex = gHitGroupContainer.GetBindIndex(material->m_hitShaderGroup);
	m_asInstances[index].accelerationStructureReference = m_blasList[meshIndex]->GetAsHandle();
	m_asInstances[index].instanceCustomIndex = index;
	m_asInstances[index].mask = 0xFF;
	m_asInstances[index].instanceShaderBindingTableRecordOffset = hitGroupIndex;
	if (material->m_mateiralTypeIndex == static_cast<uint32_t>(EMaterialType::SURFACE_TYPE_TRANSPARENT))
	{
		m_asInstances[index].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR | VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR;
	}
	else
	{
		m_asInstances[index].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR;
	}
}

void BottomLevelAsGroup::RefreshInstanceDatas(bool update)
{
	uint32_t instPerMeshCount = gRenderObjContainer.GetRenderObjectInstancePerMeshCount();
	m_asInstances.resize(instPerMeshCount);
	for (uint32_t i = 0; i < instPerMeshCount; i++)
	{
		SetInstanceData(i, update);
	}

	RefreshInstanceBufferDatas();
}

void BottomLevelAsGroup::RefreshInstanceBufferDatas()
{
	if (m_instanceCount != m_asInstances.size())
	{
		m_instanceCount = static_cast<uint32_t>(m_asInstances.size());

		if (m_instancesDeviceBuffer.IsAllocated())
		{
			m_instancesDeviceBuffer.Reset
			(
				VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				static_cast<int>(m_asInstances.size())
			);
		}
		else
		{
			m_instancesDeviceBuffer.Initialzie
			(
				VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				static_cast<int>(m_asInstances.size())
			);
		}
	}
	m_instancesDeviceBuffer.UpdateResource(m_asInstances.data(), static_cast<int>(m_asInstances.size()));
}

void BottomLevelAsGroup::RefreshBlasList(bool update)
{
	RefreshInstanceDatas(update);

	m_asGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	m_asGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	m_asGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	m_asGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	m_asGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
	m_asGeometry.geometry.instances.data.deviceAddress = m_instancesDeviceBuffer.GetDeviceMemoryAddress();
}

void BottomLevelAsGroup::OnMeshAdded(UID uid)
{
	SimpleMeshData* meshData = gGeomContainer.GetMeshFromUID(uid);
	BottomLevelAS* blas = new BottomLevelAS();
	blas->SetSourceMesh(meshData);
	m_blasList.push_back(blas);
	
	m_meshListChanged = true;
}

void BottomLevelAsGroup::OnMeshRemoved(UID uid)
{
	int index = gGeomContainer.GetMeshBindIndexFromUID(uid);
	if (index < m_blasList.size() && index != INVALID_INDEX_INT)
	{
		gVkDeviceRes.GraphicsQueueWaitIdle();

		BottomLevelAS* blas = m_blasList[index];
		if (blas != nullptr)
		{
			blas->Destroy();
			delete blas;
			blas = nullptr;
		}	
		m_blasList.erase(m_blasList.begin() + index);
	}
	
	m_meshListChanged = true;
}

void BottomLevelAsGroup::OnInstPerMeshAdded(uint32_t index)
{
	m_asInstances.emplace_back();
	SetInstanceData(index, false);

	m_instanceListChanged = true;
}

void BottomLevelAsGroup::OnInstPerMeshUpdated(uint32_t index)
{
	SetInstanceData(index, true);

	m_instanceListChanged = true;
}

void BottomLevelAsGroup::OnInstPerMeshRemoved(uint32_t index)
{
	m_asInstances.erase(m_asInstances.begin() + index);
	m_instanceListChanged = true;
}

void BottomLevelAsGroup::OnMeshUpdated(UID uid)
{
	int index = gGeomContainer.GetMeshBindIndexFromUID(uid);
	m_blasList[index]->SetBuildState(EBlasBuildState::NEED_UPDATE_BUILD);
}

void BottomLevelAsGroup::Destroy()
{
	gGeomContainer.OnMeshLoaded.Remove(m_meshLoadedCallbackHandle);
	gGeomContainer.OnMeshUnloaded.Remove(m_meshUnloadedCallbackHandle);
	gRenderObjContainer.OnInstPerMeshAdded.Remove(m_instPreMeshAddedCallbackHandle);
	gRenderObjContainer.OnInstPerMeshRemoved.Remove(m_instPerMeshRemovedCallbackHandle);

	Clear();
}

bool TopLevelAS::Build(VkCommandBuffer commandBuffer, std::vector<BottomLevelAsGroup*>& bottomLevelAsGroupList, bool updateBuild)
{
	uint32_t instanceCount = 0;
	for (auto cur : bottomLevelAsGroupList)
	{
		instanceCount += cur->GetInstanceCount();
	}

	std::vector<VkAccelerationStructureGeometryKHR> asGeomList;
	for (auto cur : bottomLevelAsGroupList)
	{
		asGeomList.push_back(cur->GetVkAccelerationStructureGeometryKHR());
	}
	VkAccelerationStructureGeometryKHR* asGeomListPtr = asGeomList.data();

	if (!updateBuild)
	{
		if (m_accelerationStructure != VK_NULL_HANDLE)
		{
			vkDestroyAccelerationStructureKHR(gLogicalDevice, m_accelerationStructure, nullptr);
			m_accelerationStructure = VK_NULL_HANDLE;
		}

		VkAccelerationStructureBuildGeometryInfoKHR asBuildGeomInfo = {};
		asBuildGeomInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		asBuildGeomInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		asBuildGeomInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		asBuildGeomInfo.geometryCount = static_cast<uint32_t>(asGeomList.size());
		asBuildGeomInfo.ppGeometries = &asGeomListPtr;

		VkAccelerationStructureBuildSizesInfoKHR asBuildSizeInfo = {};
		asBuildSizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		vkGetAccelerationStructureBuildSizesKHR
		(
			gLogicalDevice,
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&asBuildGeomInfo,
			&instanceCount,
			&asBuildSizeInfo
		);

		if (m_asMemory.IsAllocated())
		{
			m_asMemory.Destroy();
		}
		if (!m_asMemory.Initialize(static_cast<uint32_t>(asBuildSizeInfo.accelerationStructureSize), VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
		{
			return false;
		}

		VkAccelerationStructureCreateInfoKHR asCreateInfo = {};
		asCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		asCreateInfo.buffer = m_asMemory.GetBuffer();
		asCreateInfo.size = asBuildSizeInfo.accelerationStructureSize;
		asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		
		if (vkCreateAccelerationStructureKHR(gLogicalDevice, &asCreateInfo, nullptr, &m_accelerationStructure) != VkResult::VK_SUCCESS)
		{
			REPORT(EReportType::REPORT_TYPE_ERROR, "tlas create failed.");
			return false;
		}

		if (m_scratchBuffer.IsAllocated())
		{
			m_scratchBuffer.Destroy();
		}
		if (!m_scratchBuffer.Initialize(static_cast<uint32_t>(asBuildSizeInfo.buildScratchSize), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
		{
			return false;
		}
	}

	VkAccelerationStructureBuildGeometryInfoKHR asBuildGeomInfo = {};
	asBuildGeomInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	asBuildGeomInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	asBuildGeomInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
	if (updateBuild)
	{
		asBuildGeomInfo.srcAccelerationStructure = m_accelerationStructure;
	}
	asBuildGeomInfo.dstAccelerationStructure = m_accelerationStructure;
	asBuildGeomInfo.geometryCount = static_cast<uint32_t>(asGeomList.size());
	asBuildGeomInfo.ppGeometries = &asGeomListPtr;
	asBuildGeomInfo.scratchData.deviceAddress = m_scratchBuffer.GetDeviceMemoryAddress();

	VkAccelerationStructureBuildRangeInfoKHR asBuildRangeInfo;
	asBuildRangeInfo.primitiveCount = instanceCount;
	asBuildRangeInfo.primitiveOffset = 0;
	asBuildRangeInfo.firstVertex = 0;
	asBuildRangeInfo.transformOffset = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> asBuildRangeInfos = { &asBuildRangeInfo };

	
	vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &asBuildGeomInfo, asBuildRangeInfos.data());
	VkMemoryBarrier memoryBarrier;
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.pNext = nullptr;
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
	memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
	vkCmdPipelineBarrier
	(
		commandBuffer, 
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		0,
		1,
		&memoryBarrier,
		0,
		nullptr,
		0,
		nullptr
	);
	m_isBuilded = true;

	return true;
}

bool RTAccelerationStructure::Initialize(VkCommandPool cmdPool)
{
	return m_commandBufferContainer.Initialize(cmdPool);
}


void RTAccelerationStructure::Clear()
{
	m_commandBufferContainer.Clear();
	m_topLevelAs.Destroy();
	m_bottomLevelAsGroup.Clear();
}

bool RTAccelerationStructure::Build()
{
	//최초 전체빌드일때는 버퍼 만들고 대기한다
	SingleTimeCommandBuffer singleTimeCmdBuffer;
	singleTimeCmdBuffer.Begin();
	m_bottomLevelAsGroup.Build(singleTimeCmdBuffer.GetCommandBuffer());
	std::vector<BottomLevelAsGroup*> bottomLevelAsGroups = { &m_bottomLevelAsGroup };
	if (!m_topLevelAs.Build(singleTimeCmdBuffer.GetCommandBuffer(), bottomLevelAsGroups))
	{
		//top level as 빌드실패로깅
		return false;
	}
	singleTimeCmdBuffer.End();
	return true;
}

void RTAccelerationStructure::Update()
{
	if (m_bottomLevelAsGroup.IsInstanceListChanged() || m_bottomLevelAsGroup.IsMeshListChanged())
	{
		m_asBuildCommandBuffer = m_commandBufferContainer.GetCommandBuffer();
		if (m_asBuildCommandBuffer->Begin())
		{
			m_bottomLevelAsGroup.Update(m_asBuildCommandBuffer->GetCommandBuffer());

			//instance 리스트나 blas 리스트가 수정되었다면 전체 재빌드
			std::vector<BottomLevelAsGroup*> bottomLevelAsGroups = { &m_bottomLevelAsGroup };
			m_topLevelAs.Build(m_asBuildCommandBuffer->GetCommandBuffer(), bottomLevelAsGroups, !m_bottomLevelAsGroup.IsMeshListChanged());

			m_asBuildCommandBuffer->End();

			m_isPipelineResourceUpdated = m_bottomLevelAsGroup.IsMeshListChanged();
			m_hasWaitingCommandToBuild = true;
		}
	}
	else
	{
		m_bottomLevelAsGroup.Update(VK_NULL_HANDLE);
	}
	m_bottomLevelAsGroup.OnUpdateComplete();
}

void RTAccelerationStructure::Destroy()
{
	m_commandBufferContainer.Clear();
	m_topLevelAs.Destroy();
	m_bottomLevelAsGroup.Destroy();
}