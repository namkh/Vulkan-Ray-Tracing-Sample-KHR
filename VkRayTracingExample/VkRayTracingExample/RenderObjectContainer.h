#pragma once

#include "SimpleRenderObject.h"
#include "Singleton.h"

class RenderObjectContainer : public TSingleton<RenderObjectContainer>
{
public:
	RenderObjectContainer(token) 
	{
		m_renderObjList.reserve(RESOURCE_CONTAINER_INITIAL_SIZE);
		m_instList.reserve(RESOURCE_CONTAINER_INITIAL_SIZE);
		m_instPerMeshList.reserve(RESOURCE_CONTAINER_INITIAL_SIZE);
	};

public:
	SimpleRenderObject* CreateRenderObject(std::string fbxFilePath, ExampleMaterialType exampleMaterialType);
	void RemoveRenderObject(SimpleRenderObject* obj);

	SampleRenderObjectInstance* CreateRenderObjectInstance(glm::mat4& matWorld);
	void RemoveRenderObjectInstance(SampleRenderObjectInstance* instance);

	SampleRenderObjectInstancePerMesh* CreateRenderObjectInstancePerMesh(SampleRenderObjectInstance* parentInst, SimpleMeshData* meshData, SimpleMaterial* material);
	void RemoveRenderObjectInstancePerMesh(SampleRenderObjectInstancePerMesh* instancePreMesh);

	void Clear();

public:

	uint32_t GetRenderObjectCount();
	SimpleRenderObject* GetRenderObject(uint32_t index);
	int GetRenderObjectBindIndex(SimpleRenderObject* instPerMesh);

	uint32_t GetRenderObjectInstanceCount();
	SampleRenderObjectInstance* GetRenderObjectInstance(uint32_t index);
	int GetRenderObjectInstanceBindIndex(SampleRenderObjectInstance* inst);

	uint32_t GetRenderObjectInstancePerMeshCount();
	SampleRenderObjectInstancePerMesh* GetRenderObjectInstancePerMesh(uint32_t index);
	int GetRenderObjectInstancePerMeshBindIndex(SampleRenderObjectInstancePerMesh* instPerMesh);

	bool IsDirty() { return m_isDirty; }
	void SetDirty(bool isDirty) { m_isDirty = true; }

	LambdaCommandListWithOneParam<std::function<void(uint32_t)>, uint32_t> OnInstPerMeshAdded;
	LambdaCommandListWithOneParam<std::function<void(uint32_t)>, uint32_t> OnInstPerMeshRemoved;

protected:

	template <typename CreateItemType, typename IndexListType, typename ItemListType>
	CreateItemType* Create(IndexListType* idxList, ItemListType* itemList);

	template <typename RemoveItemType, typename IndexListType, typename ItemListType>
	void Remove(RemoveItemType* item, IndexListType* idxList, ItemListType* itemList);

	template <typename IndexListType, typename ItemListType>
	void RefreshTable(IndexListType* idxList, ItemListType* itemList);

private:

	std::map<UID, uint32_t> m_objIndexTable;
	std::vector<SimpleRenderObject*> m_renderObjList;

	std::map<UID, uint32_t> m_instIndexTable;
	std::vector<SampleRenderObjectInstance*> m_instList;

	std::map<UID, uint32_t> m_instPerMeshIndexTable;
	std::vector<SampleRenderObjectInstancePerMesh*> m_instPerMeshList;

	bool m_isDirty = false;
};

template <typename CreateItemType, typename IndexListType, typename ItemListType>
CreateItemType* RenderObjectContainer::Create(IndexListType* idxList, ItemListType* itemList)
{
	CreateItemType* item = new CreateItemType();
	itemList->push_back(item);
	uint32_t index = static_cast<uint32_t>(itemList->size() - 1);
	idxList->insert(std::make_pair(item->GetUID(), index));
	m_isDirty = true;
	return item;
}

template <typename RemoveItemType, typename IndexListType, typename ItemListType>
void RenderObjectContainer::Remove(RemoveItemType* item, IndexListType* idxList, ItemListType* itemList)
{
	auto iterIdxFind = idxList->find(item->GetUID());
	if (iterIdxFind != idxList->end())
	{
		uint32_t index = static_cast<uint32_t>(iterIdxFind->second);
		idxList->erase(iterIdxFind);
		(*itemList)[index]->Destroy();
		delete (*itemList)[index];
		itemList->erase(itemList->begin() + index);
		m_isDirty = true;
	}
}

template <typename IndexListType, typename ItemListType>
void RenderObjectContainer::RefreshTable(IndexListType* idxList, ItemListType* itemList)
{
	for (uint32_t i = 0; i < itemList->size(); i++)
	{
		auto iterFind = idxList->find((*itemList)[i]->GetUID());
		if (iterFind != idxList->end())
		{
			iterFind->second = i;
		}
	}
}

#define gRenderObjContainer RenderObjectContainer::Instance()