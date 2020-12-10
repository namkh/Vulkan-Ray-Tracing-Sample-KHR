#include "GeometryContainer.h"
#include "Utils.h"

SimpleGeometry* GeometryContainer::CreateGeometry(std::string& fbxFilePath)
{
	SimpleGeometry* geomData = nullptr;

	//로드 되어있는지 검색
	auto iterKeyFinded = m_geomKeyTable.find(fbxFilePath);
	if(iterKeyFinded != m_geomKeyTable.end())
	{
		auto iterIdxFinded = m_geomIndexTable.find(iterKeyFinded->second);
		if (iterIdxFinded != m_geomIndexTable.end())
		{
			if (iterIdxFinded->second < m_geomDatas.size())
			{
				geomData = m_geomDatas[iterIdxFinded->second];
			}
		}
	}

	//로드 되어있지 않다면 로드한다
	if (geomData == nullptr)
	{
		geomData = LoadGeometry(fbxFilePath);
		if (geomData == nullptr)
		{
			//로딩 실패 로깅....
		}
	}

	if (geomData != nullptr)
	{
		geomData->IncRef();
	}
	
	return geomData;
}

int GeometryContainer::GetMeshBindIndex(SimpleMeshData* meshData)
{
	return GetMeshBindIndexFromUID(meshData->GetUID());
}

int GeometryContainer::GetMeshBindIndexFromUID(UID uid)
{
	auto iterFind = m_meshIndexTable.find(uid);
	if (iterFind != m_meshIndexTable.end())
	{
		return iterFind->second;
	}
	return INVALID_INDEX_INT;
}

void GeometryContainer::RemoveUnusedGeometries()
{
	std::vector<SimpleGeometry*> m_removeList;
	for (auto& cur : m_geomDatas)
	{
		if (cur->GetRefCount() == 0)
		{	
			m_removeList.push_back(cur);
		}
	}

	for (auto cur : m_removeList)
	{
		UnloadGeometry(cur);
	}
}

void GeometryContainer::Clear()
{
	for (auto& cur : m_geomDatas)
	{
		if (cur->GetRefCount() == 0)
		{
			cur->Unload();
			delete cur;
		}
	}
	m_geomDatas.clear();
	m_geomIndexTable.clear();
}

SimpleGeometry* GeometryContainer::GetGeometry(uint32_t index)
{
	if (index < m_geomDatas.size())
	{
		return m_geomDatas[index];
	}
	return nullptr;
}

SimpleMeshData* GeometryContainer::GetMesh(uint32_t index)
{
	if (index < m_meshDatas.size())
	{
		return m_meshDatas[index];
	}
	return nullptr;
}

SimpleMeshData* GeometryContainer::GetMeshFromUID(UID uid)
{
	int index = GetMeshBindIndexFromUID(uid);
	if (index != INVALID_INDEX_INT)
	{
		return m_meshDatas[index];
	}
	return nullptr;
}

SimpleGeometry* GeometryContainer::LoadGeometry(std::string& filePath)
{
	SimpleGeometry* geometry = new SimpleGeometry();
	m_geomDatas.push_back(geometry);
	uint32_t addedIndex = static_cast<uint32_t>(m_geomDatas.size() - 1);
	UID uid = geometry->GetUID();
	geometry->Load(filePath);
	m_geomKeyTable.insert(std::make_pair(filePath, uid));
	m_geomIndexTable.insert(std::make_pair(uid, addedIndex));
	
	return geometry;
}

void GeometryContainer::UnloadGeometry(SimpleGeometry* geomData)
{
	m_geomKeyTable.erase(geomData->GetSrcFilePath());
	auto iterIdxFind = m_geomIndexTable.find(geomData->GetUID());
	if (iterIdxFind != m_geomIndexTable.end())
	{
		if (iterIdxFind->second < m_geomDatas.size())
		{
			int geomIndex = iterIdxFind->second;
			m_geomIndexTable.erase(iterIdxFind);

			m_geomDatas[geomIndex]->Unload();
			delete m_geomDatas[geomIndex];

			m_geomDatas.erase(m_geomDatas.begin() + geomIndex);

			RefreshIndexTable();
		}
	}
}

SimpleMeshData* GeometryContainer::LoadMesh(FbxGeometryData& fbxGeomData)
{
	SimpleMeshData* meshData = new SimpleMeshData();
	m_meshDatas.push_back(meshData);
	int addedIndex = static_cast<uint32_t>(m_meshDatas.size() - 1);
	meshData->Load(fbxGeomData);
	m_meshIndexTable.insert(std::make_pair(meshData->GetUID(), static_cast<uint32_t>(addedIndex)));

	OnMeshLoaded.Exec(addedIndex);

	return meshData;
}

void GeometryContainer::UnloadMesh(SimpleMeshData* geomData)
{
	auto iterIdxFind = m_meshIndexTable.find(geomData->GetUID());
	if (iterIdxFind != m_meshIndexTable.end())
	{
		if (iterIdxFind->second < m_meshDatas.size())
		{
			m_meshDatas[iterIdxFind->second]->Unload();
			delete m_meshDatas[iterIdxFind->second];
			m_meshDatas.erase(m_meshDatas.begin() + iterIdxFind->second);
			RefreshIndexTable();
		}
		OnMeshUnloaded.Exec(iterIdxFind->second);
	}
}

void GeometryContainer::RefreshIndexTable()
{
	auto iterGeomFind = m_geomIndexTable.end();
	for (int i = 0; i < m_geomDatas.size(); i++)
	{
		iterGeomFind = m_geomIndexTable.find(m_geomDatas[i]->GetUID());
		if (iterGeomFind != m_geomIndexTable.end())
		{
			iterGeomFind->second = i;
		}
	}

	auto iterMeshFind = m_meshIndexTable.end();
	for (int i = 0; i < m_meshDatas.size(); i++)
	{
		iterMeshFind = m_meshIndexTable.find(m_meshDatas[i]->GetUID());
		if (iterMeshFind != m_meshIndexTable.end())
		{
			iterMeshFind->second = i;
		}
	}
}