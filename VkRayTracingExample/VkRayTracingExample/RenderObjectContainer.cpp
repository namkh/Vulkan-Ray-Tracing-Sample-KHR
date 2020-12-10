#include "RenderObjectContainer.h"

SimpleRenderObject* RenderObjectContainer::CreateRenderObject(std::string fbxFilePath, ExampleMaterialType exampleMaterialType)
{
	SimpleRenderObject* obj = Create<SimpleRenderObject, std::map<UID, uint32_t>, std::vector<SimpleRenderObject*> >(&m_objIndexTable, &m_renderObjList);
	obj->Initialize(fbxFilePath, exampleMaterialType);
	return obj;
}

void RenderObjectContainer::RemoveRenderObject(SimpleRenderObject* obj)
{
	Remove(obj, &m_objIndexTable, &m_renderObjList);
}

SampleRenderObjectInstance* RenderObjectContainer::CreateRenderObjectInstance(glm::mat4& matWorld)
{
	SampleRenderObjectInstance* inst = Create<SampleRenderObjectInstance, std::map<UID, uint32_t>, std::vector<SampleRenderObjectInstance*> >(&m_instIndexTable, &m_instList);
	inst->Initialzie(matWorld);
	
	OnInstPerMeshAdded.Exec(static_cast<uint32_t>(m_instPerMeshList.size() - 1));

	return inst;
}

void RenderObjectContainer::RemoveRenderObjectInstance(SampleRenderObjectInstance* instance)
{
	Remove(instance, &m_instIndexTable, &m_instList);

	RefreshTable(&m_instIndexTable, &m_instList);
	RefreshTable(&m_instPerMeshIndexTable, &m_instPerMeshList);
}

SampleRenderObjectInstancePerMesh* RenderObjectContainer::CreateRenderObjectInstancePerMesh(SampleRenderObjectInstance* parentInst, SimpleMeshData* meshData, SimpleMaterial* material)
{
	SampleRenderObjectInstancePerMesh* instPerMesh = Create<SampleRenderObjectInstancePerMesh, std::map<UID, uint32_t>, std::vector<SampleRenderObjectInstancePerMesh*> >(&m_instPerMeshIndexTable, &m_instPerMeshList);
	instPerMesh->Initialize(parentInst, meshData, material);
	
	OnInstPerMeshAdded.Exec();

	return instPerMesh;
}

void RenderObjectContainer::RemoveRenderObjectInstancePerMesh(SampleRenderObjectInstancePerMesh* instancePreMesh)
{
	int removeIndex = GetRenderObjectInstancePerMeshBindIndex(instancePreMesh);
	if (removeIndex != -1)
	{
		Remove(instancePreMesh, &m_instPerMeshIndexTable, &m_instPerMeshList);

		OnInstPerMeshRemoved.Exec(static_cast<uint32_t>(removeIndex));

		RefreshTable(&m_instIndexTable, &m_instList);
		RefreshTable(&m_instPerMeshIndexTable, &m_instPerMeshList);
	}
}

void RenderObjectContainer::Clear()
{
	for (auto& cur : m_renderObjList)
	{
		if (cur != nullptr)
		{
			cur->Destroy();
			delete cur;
		}
	}
	m_renderObjList.clear();
	m_renderObjList.clear();
	m_instIndexTable.clear();
	m_instList.clear();
	m_instPerMeshIndexTable.clear();
	m_instPerMeshList.clear();
}

uint32_t RenderObjectContainer::GetRenderObjectCount()
{
	return static_cast<uint32_t>(m_renderObjList.size());
}

SimpleRenderObject* RenderObjectContainer::GetRenderObject(uint32_t index)
{
	if (index < m_renderObjList.size())
	{
		return m_renderObjList[index];
	}
	return nullptr;
}

int RenderObjectContainer::GetRenderObjectBindIndex(SimpleRenderObject* instPerMesh)
{
	if (instPerMesh != nullptr)
	{
		auto iterFind = m_instIndexTable.find(instPerMesh->GetUID());
		if (iterFind != m_instIndexTable.end())
		{
			return iterFind->second;
		}
	}
	return -1;
}

uint32_t RenderObjectContainer::GetRenderObjectInstanceCount()
{
	return static_cast<uint32_t>(m_instList.size());
}

SampleRenderObjectInstance* RenderObjectContainer::GetRenderObjectInstance(uint32_t index)
{
	if (index < m_instList.size())
	{
		return m_instList[index];
	}
	return nullptr;
}

int RenderObjectContainer::GetRenderObjectInstanceBindIndex(SampleRenderObjectInstance* inst)
{
	if (inst != nullptr)
	{
		auto iterFind = m_instIndexTable.find(inst->GetUID());
		if (iterFind != m_instIndexTable.end())
		{
			return iterFind->second;
		}
	}
	return -1;
}

int RenderObjectContainer::GetRenderObjectInstancePerMeshBindIndex(SampleRenderObjectInstancePerMesh* instPerMesh)
{
	auto iterFind = m_instPerMeshIndexTable.find(instPerMesh->GetUID());
	if (iterFind != m_instPerMeshIndexTable.end())
	{
		return iterFind->second;
	}
	return INVALID_INDEX_INT;
}

uint32_t RenderObjectContainer::GetRenderObjectInstancePerMeshCount()
{
	return static_cast<uint32_t>(m_instPerMeshList.size());
}

SampleRenderObjectInstancePerMesh* RenderObjectContainer::GetRenderObjectInstancePerMesh(uint32_t index)
{
	if (index < m_instPerMeshList.size())
	{
		return m_instPerMeshList[index];
	}
	return nullptr;
}