#pragma once

#include <map>
#include <functional>

#include "DeviceBuffers.h"
#include "Singleton.h"
#include "SimpleGeometry.h"
#include "Commands.h"

class GeometryContainer : public TSingleton<GeometryContainer>
{
friend class SimpleGeometry;
public:
	GeometryContainer(token) 
	{
		m_meshDatas.reserve(RESOURCE_CONTAINER_INITIAL_SIZE);
		m_geomDatas.reserve(RESOURCE_CONTAINER_INITIAL_SIZE);
	};
	
public:
	SimpleGeometry* CreateGeometry(std::string& filePath);
	int GetMeshBindIndex(SimpleMeshData* meshData);
	int GetMeshBindIndexFromUID(UID uid);

	void RemoveUnusedGeometries();
	void Clear();

public:
	uint32_t GetGeometryCount() { return static_cast<uint32_t>(m_geomDatas.size()); }
	SimpleGeometry* GetGeometry(uint32_t index);

	uint32_t GetMeshCount() { return static_cast<uint32_t>(m_meshDatas.size()); }
	SimpleMeshData* GetMesh(uint32_t index);
	SimpleMeshData* GetMeshFromUID(UID uid);


protected:

	SimpleGeometry* LoadGeometry(std::string& filePath);
	void UnloadGeometry(SimpleGeometry* geomData);

	SimpleMeshData* LoadMesh(FbxGeometryData& fbxGeomData);
	void UnloadMesh(SimpleMeshData* geomData);

	void RefreshIndexTable();

public:

	LambdaCommandListWithOneParam<std::function<void(uint32_t)>, uint32_t> OnMeshLoaded;
	LambdaCommandListWithOneParam<std::function<void(uint32_t)>, uint32_t> OnMeshUnloaded;
	
private:
	std::map<std::string, UID> m_geomKeyTable;
	std::map<UID, uint32_t> m_geomIndexTable;
	std::vector<SimpleGeometry*> m_geomDatas;

	std::map<UID, uint32_t> m_meshIndexTable;
	std::vector<SimpleMeshData*> m_meshDatas;
};

#define gGeomContainer GeometryContainer::Instance()

