#include "SimpleRenderObject.h"
#include "RenderObjectContainer.h"

void SampleRenderObjectInstancePerMesh::Initialize(SampleRenderObjectInstance* parentInstance, SimpleMeshData* meshData, SimpleMaterial* material)
{
	m_meshData = meshData;
	m_material = material;
	m_parentInstance = parentInstance;
}

void SampleRenderObjectInstancePerMesh::Destroy()
{
	OnMeshUpdated.Clear();
	if (m_overrideMaterial != nullptr)
	{
		gMaterialContainer.RemoveMaterial(m_overrideMaterial);
		m_overrideMaterial = nullptr;
	}
}

void SampleRenderObjectInstancePerMesh::SetOverrideMaterial(ExampleMaterialType type)
{
	m_overrideMaterial = gMaterialContainer.CreateMaterial(type);
}

SimpleMeshData* SampleRenderObjectInstancePerMesh::GetMeshData()
{
	return m_meshData;
}

SimpleMaterial* SampleRenderObjectInstancePerMesh::GetMaterial()
{
	if (m_overrideMaterial != nullptr)
	{
		return m_overrideMaterial;
	}
	return m_material;
}

void SampleRenderObjectInstancePerMesh::OnMeshUpdate()
{
	OnMeshUpdated.Exec();
}

void SampleRenderObjectInstance::Initialzie(glm::mat4 worldMat)
{
	m_worldMatrix = worldMat;
}

void SampleRenderObjectInstance::Destroy()
{
	OnInstanceUpdated.Clear();
	for (auto& cur : m_instancePerMesh)
	{
		gRenderObjContainer.RemoveRenderObjectInstancePerMesh(cur);
	}
}

void SampleRenderObjectInstance::CreateInstancePerMesh(SimpleMeshData* meshData, SimpleMaterial* material)
{	
	m_instancePerMesh.push_back(gRenderObjContainer.CreateRenderObjectInstancePerMesh(this, meshData, material));
}

void SampleRenderObjectInstance::SetWorldMatrix(glm::mat4& matWorld, bool isUpdate) 
{
	m_worldMatrix = matWorld;
	if (isUpdate)
	{
		for (auto& cur : m_instancePerMesh)
		{
			int index = gRenderObjContainer.GetRenderObjectInstancePerMeshBindIndex(cur);
			if (index != INVALID_INDEX_INT)
			{
				OnInstanceUpdated.Exec(index);
			}
		}	
	}
}

void SampleRenderObjectInstance::SetOverrideMaterial(ExampleMaterialType type)
{
	for (auto& cur : m_instancePerMesh)
	{
		cur->SetOverrideMaterial(type);
	}
}

SampleRenderObjectInstancePerMesh* SampleRenderObjectInstance::GetInstancePerMesh(uint32_t index)
{
	if (index < m_instancePerMesh.size())
	{
		return m_instancePerMesh[index];
	}
	return nullptr;
}

void SimpleRenderObject::Initialize(std::string fbxFilePath, ExampleMaterialType exampleMaterialType)
{
	if (m_geometry != nullptr)
	{
		m_geometry->Destroy();
		m_geometry = nullptr;
	}
	m_geometry = gGeomContainer.CreateGeometry(fbxFilePath);

	if (m_material != nullptr)
	{
		gMaterialContainer.RemoveMaterial(m_material);
	}
	m_material = gMaterialContainer.CreateMaterial(exampleMaterialType);
}

void SimpleRenderObject::Destroy()
{
	for (auto& curInstance : m_instances)
	{
		gRenderObjContainer.RemoveRenderObjectInstance(curInstance);
	}
	if (m_geometry != nullptr)
	{
		m_geometry->Destroy();
		m_geometry = nullptr;
	}
	if (m_material != nullptr)
	{
		gMaterialContainer.RemoveMaterial(m_material);
	}
}
	
SampleRenderObjectInstance* SimpleRenderObject::CreateInstance(glm::mat4 matWorld)
{
	SampleRenderObjectInstance* inst = nullptr;

	if (m_geometry != nullptr && m_material != nullptr)
	{
		inst = gRenderObjContainer.CreateRenderObjectInstance(matWorld);
		m_instances.push_back(inst);
		m_indexTable.insert(std::make_pair(inst->GetUID(), static_cast<uint32_t>(m_instances.size()) - 1));
		int instancePerMeshIndex = -1;
		for (uint32_t i = 0; i < m_geometry->GetMeshCount(); i++)
		{
			SimpleMeshData* meshData = m_geometry->GetMesh(i);
			if (meshData != nullptr)
			{
				inst->CreateInstancePerMesh(meshData, m_material);
			}
		}
	}
	return inst;
}

void SimpleRenderObject::RemoveInstance(SampleRenderObjectInstance* subMeshInstance)
{
	auto iterFind = m_indexTable.find(subMeshInstance->GetUID());
	if (iterFind != m_indexTable.end())
	{
		uint32_t index = iterFind->second;
		m_indexTable.erase(iterFind);
		m_instances.erase(m_instances.begin() + index);
	}
	gRenderObjContainer.RemoveRenderObjectInstance(subMeshInstance);
}