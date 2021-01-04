#pragma once

#include "GeometryContainer.h"
#include "MaterialContainer.h"
#include "Utils.h"
class SampleRenderObjectInstance;

class SampleRenderObjectInstancePerMesh : public UniqueIdentifier
{
friend class RenderObjectContainer;
public:
	SampleRenderObjectInstancePerMesh() : UniqueIdentifier() {}
public:
	void SetOverrideMaterial(ExampleMaterialType type);

	SimpleMeshData* GetMeshData();
	SimpleMaterial* GetMaterial();
	SampleRenderObjectInstance* GetParentInstance() { return m_parentInstance; }

	void OnMeshUpdate();

	LambdaCommandListWithOneParam<std::function<void(VkCommandBuffer)>, VkCommandBuffer> OnMeshUpdated;

protected:
	void Initialize(SampleRenderObjectInstance* parentInstance,  SimpleMeshData* m_meshData, SimpleMaterial* m_material);
	void Destroy();

protected:
	SimpleMeshData* m_meshData = nullptr;
	SimpleMaterial* m_material = nullptr;
	SimpleMaterial* m_overrideMaterial = nullptr;
	SampleRenderObjectInstance* m_parentInstance = nullptr;
};

class SampleRenderObjectInstance : public UniqueIdentifier
{
friend class RenderObjectContainer;
public:
	SampleRenderObjectInstance() : UniqueIdentifier() {}
public:
	void SetOverrideMaterial(ExampleMaterialType type);

	uint32_t GetInstancePerMeshCount() { return static_cast<uint32_t>(m_instancePerMesh.size()); }

	SampleRenderObjectInstancePerMesh* GetInstancePerMesh(uint32_t index);
	void CreateInstancePerMesh(SimpleMeshData* meshData, SimpleMaterial* material);

	void SetWorldMatrix(glm::mat4& matWorld, bool isUpdate);
	glm::mat4& GetWorldMatrix() { return m_worldMatrix; }

	LambdaCommandListWithOneParam<std::function<void(uint32_t)>, uint32_t> OnInstanceUpdated;

protected:
	void Initialzie(glm::mat4 worldMat);
	void Destroy();

private:
	glm::mat4 m_worldMatrix = glm::mat4(1.0f);
	std::vector<SampleRenderObjectInstancePerMesh*> m_instancePerMesh = {};
};

class SimpleRenderObject : public UniqueIdentifier
{
friend class RenderObjectContainer;
public:
	SimpleRenderObject() : UniqueIdentifier() {}
public:
	SampleRenderObjectInstance* CreateInstance(glm::mat4 matWorld);
	void RemoveInstance(SampleRenderObjectInstance* subMeshInstance);

protected:
	void Initialize(std::string fbxFilePath, ExampleMaterialType exampleMaterialType);
	void Destroy();

private:

	SimpleGeometry* m_geometry = nullptr;
	SimpleMaterial* m_material = nullptr;
	uint32_t m_materialIndex = 0;

	std::map<UID, uint32_t> m_indexTable = {};
	std::vector<SampleRenderObjectInstance*> m_instances = {};
};