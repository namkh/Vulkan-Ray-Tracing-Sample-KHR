#pragma once

#include <map>

#include "DeviceBuffers.h"
#include "SimpleMaterial.h"
#include "RenderObjectContainer.h"
#include "GeometryContainer.h"
#include "CommandBuffers.h"

class RayTracingAccelerationStructureBase
{
public:
	RayTracingAccelerationStructureBase() {};
	virtual ~RayTracingAccelerationStructureBase() {};

public:
	void Destroy();
	void Reset();
	virtual VkAccelerationStructureKHR& GetAccelerationStructure() { return m_accelerationStructure; }

	VkDeviceAddress GetAsHandle() { return m_handle; }

protected:
	VkAccelerationStructureKHR	m_accelerationStructure = VK_NULL_HANDLE;
	VkTransformMatrixKHR		m_transformMatrix = {};
	RayTracingScratchBuffer		m_scratchBuffer;
	BufferData					m_asMemory;

	VkDeviceAddress m_handle = 0;
	bool m_isBuilded = false;
};

enum class EBlasBuildState
{
	NEED_BUILD,
	NEED_UPDATE_BUILD,
	BUILDED
};

class BottomLevelAS : public RayTracingAccelerationStructureBase
{
public:
	BottomLevelAS() {};
	virtual ~BottomLevelAS() {};
public:
	bool Build(VkCommandBuffer commandBuffer, bool update = false);

	VkAccelerationStructureKHR GetBottomLevelAs() { return m_accelerationStructure; }
	
	void SetBuildState(EBlasBuildState state) { m_buildState = state; }
	EBlasBuildState GetBuildState() { return m_buildState; }

	void SetSourceMesh(SimpleMeshData* sourceMesh) { m_sourceMesh = sourceMesh; }
	
protected:

	SimpleMeshData* m_sourceMesh = nullptr;
	VkAccelerationStructureGeometryKHR m_bottomLevelAsGeometry = {};
	uint32_t m_primitiveCount = 0;
	EBlasBuildState m_buildState = EBlasBuildState::NEED_BUILD;
};

class BottomLevelAsGroup
{
public:
	void Clear();
	void Build(VkCommandBuffer commandBuffer);
	void Update(VkCommandBuffer commandBuffer = VK_NULL_HANDLE);
	void Destroy();

protected:
	void SetInstanceData(int index, bool update = false);

	void RefreshInstanceDatas(bool update = false);
	void RefreshInstanceBufferDatas();
	void RefreshBlasList(bool update = false);

public:
	VkAccelerationStructureGeometryKHR& GetVkAccelerationStructureGeometryKHR() { return m_asGeometry; }
	StructuredBufferData<VkAccelerationStructureInstanceKHR>& GetAsInstancesDeviceBuffer() { return m_instancesDeviceBuffer; }

	int GetInstanceCount()			{ return m_instanceCount; }
	bool IsBuilded()				{ return m_isBuilded; }
	bool IsInstanceListChanged()	{ return m_instanceListChanged; }
	bool IsMeshListChanged()		{ return m_meshListChanged; }
	void OnUpdateComplete()	
	{
		m_meshListChanged = false; 
		m_instanceListChanged = false;
	}

public:
	void OnMeshAdded(UID uid);
	void OnMeshRemoved(UID uid);

	void OnInstPerMeshAdded(uint32_t index);
	void OnInstPerMeshUpdated(uint32_t index);
	void OnInstPerMeshRemoved(uint32_t index);

	void OnMeshUpdated(UID uid);

private:
	std::vector<BottomLevelAS*> m_blasList;
	
//	VkAccelerationStructureCreateGeometryTypeInfoKHR m_topLevelAsCreateGeomTypeInfo = {};
	std::vector<VkAccelerationStructureInstanceKHR> m_asInstances;
	StructuredBufferData<VkAccelerationStructureInstanceKHR> m_instancesDeviceBuffer = {};
	VkAccelerationStructureGeometryKHR m_asGeometry = {};

	uint32_t m_instanceCount = -1;

	bool m_isBuilded = false;
	bool m_instanceListChanged = false;
	bool m_meshListChanged = false;

	CommandHandle m_meshLoadedCallbackHandle = INVALID_COMMAND_HANDLE;
	CommandHandle m_meshUnloadedCallbackHandle = INVALID_COMMAND_HANDLE;
	CommandHandle m_instPreMeshAddedCallbackHandle = INVALID_COMMAND_HANDLE;
	CommandHandle m_instPerMeshRemovedCallbackHandle = INVALID_COMMAND_HANDLE;
};

class TopLevelAS : public RayTracingAccelerationStructureBase
{
public:
	bool Build(VkCommandBuffer commandBuffer, std::vector<BottomLevelAsGroup*>& bottomLevelAsGroupList, bool updateBuild = false);

protected:
	uint64_t m_handle = 0;
	bool m_isBuilded = false;
};

class RTAccelerationStructure
{
public:
	
	bool Initialize(VkCommandPool cmdPool);
	void Clear();
	bool Build();
	void Update();
	void Destroy();

	TopLevelAS& GetTopLevelAs() { return m_topLevelAs; }
	CommandBuffer* GetBuildCommandBuffer() { return m_asBuildCommandBuffer; }

	bool HasWaitingCommandToBuild() { return m_hasWaitingCommandToBuild; }
	void NotifyBuildCommandSubmitted() { m_hasWaitingCommandToBuild = false; }

	bool IsPipelineResourceUpdated() { return m_isPipelineResourceUpdated; }

public:

	BottomLevelAsGroup	m_bottomLevelAsGroup;
	TopLevelAS			m_topLevelAs;
	CommandBuffer*		m_asBuildCommandBuffer;
	DynamicCommandBufferContainer m_commandBufferContainer;

	bool m_hasWaitingCommandToBuild = false;
	bool m_isPipelineResourceUpdated = false;
};
