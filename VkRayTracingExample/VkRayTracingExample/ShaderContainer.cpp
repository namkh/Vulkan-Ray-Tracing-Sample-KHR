
#include "ShaderContainer.h"

SimpleShader* ShaderContainer::CreateShader(ERTShaderType shaderType, std::string& filePath)
{
	SimpleShader* shader = nullptr;
	ERTShaderGroupType groupType = GetShaderGroupType(shaderType);
	//로드 되어있는지 검색
	auto iterKeyFinded = m_keyTable.find(filePath);
	if (iterKeyFinded != m_keyTable.end())
	{
		auto iterListFinded = m_shaderDatas.find(groupType);
		if (iterListFinded != m_shaderDatas.end())
		{
			auto iterShaderFinded = iterListFinded->second.find(iterKeyFinded->second);
			if (iterShaderFinded != iterListFinded->second.end())
			{
				shader = iterShaderFinded->second;	
			}
		}
	}

	//로드 되어있지 않다면 로드한다
	if (shader == nullptr)
	{
		shader = LoadShader(shaderType, filePath);
		if (shader == nullptr)
		{
			//로딩 실패 로깅....
		}
	}

	if (shader != nullptr)
	{
		shader->IncRef();
	}

	return shader;
}

int ShaderContainer::GetBindIndex(SimpleShader* shader)
{
	uint32_t rayGenOffset = 0;
	uint32_t missOffset = 0;
	uint32_t hitOffset = 0;
	auto iterRgenList = m_shaderDatas.find(SHADER_GROUP_TYPE_RAY_GEN);
	if (iterRgenList != m_shaderDatas.end())
	{
		rayGenOffset += static_cast<uint32_t>(iterRgenList->second.size());
	}

	auto iterMissList = m_shaderDatas.find(SHADER_GROUP_TYPE_MISS);
	if (iterMissList != m_shaderDatas.end())
	{	
		missOffset += static_cast<uint32_t>(iterMissList->second.size());
	}

	auto iterHitList = m_shaderDatas.find(SHADER_GROUP_TYPE_HIT);
	if (iterHitList != m_shaderDatas.end())
	{
		hitOffset += static_cast<uint32_t>(iterHitList->second.size());
	}

	uint32_t offset = 0;
	ERTShaderGroupType groupType = GetShaderGroupType(shader->GetShaderType());
	switch(groupType)
	{
		case SHADER_GROUP_TYPE_MISS :
		{
			offset = rayGenOffset;
		}
		break;
		case SHADER_GROUP_TYPE_HIT :
		{
			offset = rayGenOffset + missOffset;
		}
		break;
		case SHADER_GROUP_TYPE_CALLABLE :
		{
			offset = rayGenOffset + missOffset + hitOffset;
		}
		break;
	}

	int index = INVALID_INDEX_INT;

	groupType = GetShaderGroupType(shader->GetShaderType());
	auto iterListFinded = m_shaderDatas.find(groupType);
	if (iterListFinded != m_shaderDatas.end())
	{
		auto iterShaderFind = iterListFinded->second.find(shader->GetUID());
		if (iterShaderFind != iterListFinded->second.end())
		{
			int pos = static_cast<int>(std::distance(iterListFinded->second.begin(), iterShaderFind));
			index = offset + pos;
		}
	}

	return index;
}

void ShaderContainer::RemoveUnusedShaders()
{
	std::vector<SimpleShader*> m_removeList;
	for (auto& curList : m_shaderDatas)
	{
		for (auto& curShader : curList.second)
		{
			if (curShader.second->GetRefCount() == 0)
			{
				m_removeList.push_back(curShader.second);
			}
		}
	}

	for (auto& cur : m_removeList)
	{
		UnloadShader(cur);
	}
}

void ShaderContainer::Clear()
{
	for (auto& curList : m_shaderDatas)
	{
		for (auto& curShader : curList.second)
		{
			if (curShader.second->GetRefCount() == 0)
			{
				m_keyTable.erase(curShader.second->GetSrcFilePath());
				curShader.second->Unload();
				delete curShader.second;
			}
		}
	}
	m_shaderDatas.clear();
}

uint32_t ShaderContainer::GetShaderCount()
{
	uint32_t shaderCount = 0;
	for (uint32_t i = 0; i < static_cast<uint32_t>(SHADER_GROUP_TYPE_END); i++)
	{
		auto iterFind = m_shaderDatas.find(static_cast<ERTShaderGroupType>(i));
		if (iterFind != m_shaderDatas.end())
		{
			shaderCount += static_cast<uint32_t>(iterFind->second.size());
		}
	}

	return shaderCount;
}

uint32_t ShaderContainer::GetShaderCount(ERTShaderGroupType shaderType)
{
	auto iterFind = m_shaderDatas.find(shaderType);
	if (iterFind != m_shaderDatas.end())
	{
		return static_cast<uint32_t>(iterFind->second.size());
	}
	return 0;
}

SimpleShader* ShaderContainer::GetShader(ERTShaderGroupType shaderType, uint32_t index)
{
	auto iterFind = m_shaderDatas.find(shaderType);
	if (iterFind != m_shaderDatas.end())
	{
		if (index < iterFind->second.size())
		{
			auto pos = iterFind->second.begin();
			std::advance(pos, index);
			return pos->second;
		}
	}
	return nullptr;
}

SimpleShader* ShaderContainer::LoadShader(ERTShaderType shaderType, std::string& filePath)
{
	ERTShaderGroupType groupType = GetShaderGroupType(shaderType);
	auto iterListFind = m_shaderDatas.find(groupType);
	std::unordered_map<UID, SimpleShader*>* shaderList;
	if (iterListFind == m_shaderDatas.end())
	{
		m_shaderDatas.insert(std::make_pair(groupType, std::unordered_map<UID, SimpleShader*>() = {}));
		m_shaderDatas[groupType].reserve(RESOURCE_CONTAINER_INITIAL_SIZE);
		shaderList = &m_shaderDatas[groupType];
	}
	else
	{
		shaderList = &iterListFind->second;
	}

	SimpleShader* shader = new SimpleShader();
	shaderList->insert(std::make_pair(shader->GetUID(), shader));
	if (!shader->Load(shaderType, filePath))
	{
		REPORT(EReportType::REPORT_TYPE_LOG, "Shader load failed");
		return nullptr;
	}

	UID uid = shader->GetUID();
	m_keyTable.insert(std::make_pair(filePath, uid));
	
	return shader;
}

void ShaderContainer::UnloadShader(SimpleShader* shader)
{
	
	ERTShaderGroupType shaderGroupType = GetShaderGroupType(shader->GetShaderType());
	auto iterListFind = m_shaderDatas.find(shaderGroupType);
	if (iterListFind != m_shaderDatas.end())
	{
		UID indexTableUid = shader->GetUID();
		auto iterShaderFind = iterListFind->second.find(indexTableUid);
		if (iterShaderFind != iterListFind->second.end())
		{
			m_keyTable.erase(iterShaderFind->second->GetSrcFilePath());
			iterShaderFind->second->Unload();
			delete iterShaderFind->second;
			iterListFind->second.erase(indexTableUid);
		}
	}
}

ERTShaderGroupType ShaderContainer::GetShaderGroupType(ERTShaderType shaderType)
{
	ERTShaderGroupType groupType = SHADER_GROUP_TYPE_INVALID;
	switch (shaderType)
	{
		case SHADER_TYPE_RAY_GEN :
		{
			groupType = SHADER_GROUP_TYPE_RAY_GEN;
		}
		break;
		case SHADER_TYPE_MISS:
		{
			groupType = SHADER_GROUP_TYPE_MISS;
		}
		break;
		case SHADER_TYPE_CALLABLE:
		{
			groupType = SHADER_GROUP_TYPE_CALLABLE;
		}
		break;
		case SHADER_TYPE_ANY_HIT:
		case SHADER_TYPE_INTERSECTION:
		case SHADER_TYPE_CLOSET_HIT:
		{
			groupType = SHADER_GROUP_TYPE_HIT;
		}
		break;
	};

	return groupType;
}


RtHitShaderGroup* RayHitGroupContainer::CreateHitGroup(std::string& closetHitFilePath,
													   std::string& anyHitFilePath,
													   std::string& intersectionFilePath)
{
	RtHitShaderGroup* hitGroup = nullptr;
	HitGroupKey key(intersectionFilePath, anyHitFilePath, closetHitFilePath);

	auto iterKeyFind = m_keyTable.find(key);
	if (iterKeyFind != m_keyTable.end())
	{
		auto iterHitGroupFind = m_hitGroupList.find(iterKeyFind->second);
		if (iterHitGroupFind != m_hitGroupList.end())
		{
			hitGroup = iterHitGroupFind->second;
		}
	}

	if (hitGroup == nullptr)
	{
		RtHitShaderGroup* newHitGroup = new RtHitShaderGroup();
		UID uid = newHitGroup->GetUID();
		newHitGroup->IncRef();
		m_hitGroupList.insert(std::make_pair(uid, newHitGroup));
		if (!closetHitFilePath.empty())
		{
			m_hitGroupList[uid]->LoadShader(SHADER_TYPE_CLOSET_HIT, closetHitFilePath);
		}
		if (!anyHitFilePath.empty())
		{
			m_hitGroupList[uid]->LoadShader(SHADER_TYPE_ANY_HIT, anyHitFilePath);
		}
		if (!intersectionFilePath.empty())
		{
			m_hitGroupList[uid]->LoadShader(SHADER_TYPE_INTERSECTION, intersectionFilePath);
		}
		hitGroup = m_hitGroupList[uid];
		m_keyTable.insert(std::make_pair(key, hitGroup->GetUID()));
	}
	return hitGroup;
}

void RayHitGroupContainer::RemoveUnusedShaderGroups()
{
	std::vector<RtHitShaderGroup*> m_removeList;
	for (auto& cur : m_hitGroupList)
	{
		if (cur.second->GetRefCount() == 0)
		{
			m_removeList.push_back(cur.second);
		}
	}

	for (auto cur : m_removeList)
	{
		HitGroupKey key = GetKeyFromGroup(cur);
		auto iterFind = m_keyTable.find(key);
		if (iterFind != m_keyTable.end())
		{
			m_keyTable.erase(key);
		}
		UID uid = cur->GetUID();
		auto iterHitGroupFind = m_hitGroupList.find(uid);
		if (iterHitGroupFind != m_hitGroupList.end())
		{
			if (iterHitGroupFind->second != nullptr)
			{
				iterHitGroupFind->second->Destroy();
				delete iterHitGroupFind->second;
				m_hitGroupList.erase(uid);
			}
		}
	}
}

void RayHitGroupContainer::Clear()
{
	for (auto& cur : m_hitGroupList)
	{
		if (cur.second->GetRefCount() == 0)
		{
			cur.second->Destroy();
			delete cur.second;
		}
	}
	m_hitGroupList.clear();
	m_keyTable.clear();
}

int RayHitGroupContainer::GetBindIndex(RtHitShaderGroup* hitShaderGroup)
{
	auto iterFind = m_hitGroupList.find(hitShaderGroup->GetUID());
	if (iterFind != m_hitGroupList.end())
	{
		return static_cast<int>(std::distance(m_hitGroupList.begin(), iterFind));
	}
	return INVALID_INDEX_INT;
}

RtHitShaderGroup* RayHitGroupContainer::GetHitGroup(int index)
{
	if (index < m_hitGroupList.size())
	{
		auto pos = m_hitGroupList.begin();
		std::advance(pos, index);
		return pos->second;
	}
	return nullptr;
}

RayHitGroupContainer::HitGroupKey RayHitGroupContainer::GetKeyFromGroup(RtHitShaderGroup* group)
{
	SimpleShader* closetHitShader = group->GetShader(SHADER_TYPE_CLOSET_HIT);
	SimpleShader* anyHitShader = group->GetShader(SHADER_TYPE_ANY_HIT);
	SimpleShader* intersectionShader = group->GetShader(SHADER_TYPE_INTERSECTION);

	std::string closetHitPath = "";
	if (closetHitShader != nullptr)
	{
		closetHitPath = closetHitShader->GetSrcFilePath();
	}
	std::string anyHitPath = "";
	if (anyHitShader != nullptr)
	{
		anyHitPath = anyHitShader->GetSrcFilePath();
	}
	std::string intersectionPath = "";
	if (intersectionShader != nullptr)
	{
		intersectionPath = intersectionShader->GetSrcFilePath();
	}

	return HitGroupKey(intersectionPath, anyHitPath, closetHitPath);
}
