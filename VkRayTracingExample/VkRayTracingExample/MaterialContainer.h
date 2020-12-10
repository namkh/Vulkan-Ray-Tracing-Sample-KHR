#pragma once

#include "SimpleMaterial.h"
#include "Singleton.h"

class MaterialContainer : public TSingleton<MaterialContainer>
{
public:
	MaterialContainer(token) 
	{
		m_materialList.reserve(RESOURCE_CONTAINER_INITIAL_SIZE);
	};

public:
	SimpleMaterial* CreateMaterial(ExampleMaterialType matType);
	void RemoveMaterial(SimpleMaterial* material);
	int GetBindIndex(SimpleMaterial* material);

	void Clear();

	uint32_t GetMaterialCount() { return static_cast<uint32_t>(m_materialList.size()); }
	SimpleMaterial* GetMaterial(int index);

protected:
	void RefreshIndexTable();

private:
	std::map<UID, uint32_t> m_materialIndexTable;
	std::vector<SimpleMaterial*> m_materialList;
};

#define gMaterialContainer MaterialContainer::Instance()

//TODO : hard coded
class ExampleMaterialCreateCode : public TSingleton<ExampleMaterialCreateCode>
{
public:
	ExampleMaterialCreateCode(token) {};
public:
	void CreateExampleMaterial(ExampleMaterialType type, SimpleMaterial* mat);
};

