#pragma once

#include "SimpleShader.h"
#include "Singleton.h"

#include <unordered_map>

class SimpleShader;

class ShaderContainer : public TSingleton<ShaderContainer>
{
public:
	ShaderContainer(token) { };

public:
	SimpleShader* CreateShader(ERTShaderType shaderType, std::string& filePath);
	int GetBindIndex(SimpleShader* shader);

	void RemoveUnusedShaders();
	void Clear();

public:

	uint32_t GetShaderCount();
	uint32_t GetShaderCount(ERTShaderGroupType shaderType);
	SimpleShader* GetShader(ERTShaderGroupType shaderType, uint32_t index);

protected:
	SimpleShader* LoadShader(ERTShaderType shaderType, std::string& filePath);
	void UnloadShader(SimpleShader* shader);

	void RefreshIndexTable();

	ERTShaderGroupType GetShaderGroupType(ERTShaderType shaderType);

private:

	//rgen shader + miss shader + hit shaders
	//따로관리 되어야한다.
	std::map<std::string, UID> m_keyTable;
	std::unordered_map<ERTShaderGroupType, std::unordered_map<UID, SimpleShader*> > m_shaderDatas;
};

#define gShaderContainer ShaderContainer::Instance()

class RayHitGroupContainer : public TSingleton<RayHitGroupContainer>
{
public:
	RayHitGroupContainer(token) 
	{
		m_hitGroupList.reserve(RESOURCE_CONTAINER_INITIAL_SIZE);
	};

private:
	struct HitGroupKey
	{
		HitGroupKey(std::string& intersectionFilePath, std::string& anyHitFilePath, std::string& closetHitFilePath)
			: IntersectionFilePath(intersectionFilePath)
			, AnyHitFilePath(anyHitFilePath)
			, ClosetHitFilePath(closetHitFilePath) {}

		bool operator<(const HitGroupKey& right) const
		{
			return 
			IntersectionFilePath < right.IntersectionFilePath ||
			(IntersectionFilePath == right.IntersectionFilePath && AnyHitFilePath < right.AnyHitFilePath) ||
			(IntersectionFilePath == right.IntersectionFilePath && AnyHitFilePath == right.AnyHitFilePath && ClosetHitFilePath < right.ClosetHitFilePath);
		}

		std::string IntersectionFilePath = "";
		std::string AnyHitFilePath = "";
		std::string ClosetHitFilePath = "";
	};

public:
	RtHitShaderGroup* CreateHitGroup(std::string& closetHitFilePath,
									 std::string& anyHitFilePath,
									 std::string& intersectionFilePath);
	void RemoveUnusedShaderGroups();
	void Clear();

	int GetBindIndex(RtHitShaderGroup* hitShaderGroup);

	uint32_t GetHitGroupCount() { return static_cast<int>(m_hitGroupList.size()); }
	RtHitShaderGroup* GetHitGroup(int index);

protected:
	HitGroupKey GetKeyFromGroup(RtHitShaderGroup* group);

private:

	std::map<HitGroupKey, UID> m_keyTable;
	std::unordered_map<UID, RtHitShaderGroup*> m_hitGroupList;
};

#define gHitGroupContainer RayHitGroupContainer::Instance()