
#include "ShaderContainer.h"

SimpleShader* ShaderContainer::CreateShader(ERTShaderType shaderType, std::string& filePath)
{
	SimpleShader* shader = nullptr;
	ERTShaderGroupType groupType = GetShaderGroupType(shaderType);
	//로드 되어있는지 검색
	auto iterKeyFinded = m_keyTable.find(filePath);
	if (iterKeyFinded != m_keyTable.end())
	{
		auto iterIndexFinded = m_indexTable.find(iterKeyFinded->second);

		if (iterIndexFinded != m_indexTable.end())
		{
			auto iterFind = m_shaderList.find(groupType);
			if (iterFind != m_shaderList.end())
			{
				if (iterIndexFinded->second < iterFind->second.size())
				{
					shader = iterFind->second[iterIndexFinded->second];
				}
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
	auto iterRgenList = m_shaderList.find(SHADER_GROUP_TYPE_RAY_GEN);
	if (iterRgenList != m_shaderList.end())
	{
		rayGenOffset += static_cast<uint32_t>(iterRgenList->second.size());
	}

	auto iterMissList = m_shaderList.find(SHADER_GROUP_TYPE_MISS);
	if (iterMissList != m_shaderList.end())
	{	
		missOffset += static_cast<uint32_t>(iterMissList->second.size());
	}

	auto iterHitList = m_shaderList.find(SHADER_GROUP_TYPE_HIT);
	if (iterHitList != m_shaderList.end())
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
	auto iterIdxFind = m_indexTable.find(shader->GetUID());
	if (iterIdxFind != m_indexTable.end())
	{
		index = offset + iterIdxFind->second;
	}
	
	return index;
}

void ShaderContainer::RemoveUnusedShaders()
{
	std::vector<SimpleShader*> m_removeList;
	for (auto& curList : m_shaderList)
	{
		for (auto& curShader : curList.second)
		{
			if (curShader->GetRefCount() == 0)
			{
				m_removeList.push_back(curShader);
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
	for (auto& curList : m_shaderList)
	{
		for (auto& curShader : curList.second)
		{
			if (curShader->GetRefCount() == 0)
			{
				UnloadShader(curShader);
			}
		}
	}
	m_shaderList.clear();
}

uint32_t ShaderContainer::GetShaderCount()
{
	uint32_t shaderCount = 0;
	for (uint32_t i = 0; i < static_cast<uint32_t>(SHADER_GROUP_TYPE_END); i++)
	{
		auto iterFind = m_shaderList.find(static_cast<ERTShaderGroupType>(i));
		if (iterFind != m_shaderList.end())
		{
			shaderCount += static_cast<uint32_t>(iterFind->second.size());
		}
	}

	return shaderCount;
}

uint32_t ShaderContainer::GetShaderCount(ERTShaderGroupType shaderType)
{
	auto iterFind = m_shaderList.find(shaderType);
	if (iterFind != m_shaderList.end())
	{
		return static_cast<uint32_t>(iterFind->second.size());
	}
	return 0;
}

SimpleShader* ShaderContainer::GetShader(ERTShaderGroupType shaderType, uint32_t index)
{
	auto iterFind = m_shaderList.find(shaderType);
	if (iterFind != m_shaderList.end())
	{
		if (index < iterFind->second.size())
		{
			return iterFind->second[index];
		}
	}
	return 0;
}

SimpleShader* ShaderContainer::LoadShader(ERTShaderType shaderType, std::string& filePath)
{
	ERTShaderGroupType groupType = GetShaderGroupType(shaderType);
	auto iterListFind = m_shaderList.find(groupType);
	std::vector<SimpleShader*>* shaderList;
	if (iterListFind == m_shaderList.end())
	{
		m_shaderList.insert(std::make_pair(groupType, std::vector<SimpleShader*>() = {}));
		m_shaderList[groupType].reserve(RESOURCE_CONTAINER_INITIAL_SIZE);
		shaderList = &m_shaderList[groupType];
	}
	else
	{
		shaderList = &iterListFind->second;
	}

	SimpleShader* shader = new SimpleShader();
	shaderList->push_back(shader);

	if (!shader->Load(shaderType, filePath))
	{
		//셰이더 로딩실패 로깅
		return nullptr;
	}

	UID uid = shader->GetUID();
	m_keyTable.insert(std::make_pair(filePath, uid));
	m_indexTable.insert(std::make_pair(uid, static_cast<uint32_t>(shaderList->size() - 1)));
	
	return shader;
}

void ShaderContainer::UnloadShader(SimpleShader* shader)
{
	UID indexTableUid = shader->GetUID();
	auto iterIdxFind = m_indexTable.find(indexTableUid);
	if (iterIdxFind != m_indexTable.end())
	{
		uint32_t index = iterIdxFind->second;
		ERTShaderGroupType shaderGroupType = GetShaderGroupType(shader->GetShaderType());
		auto iterListFind = m_shaderList.find(shaderGroupType);
		if (iterListFind != m_shaderList.end())
		{
			if (index < iterListFind->second.size() && iterListFind->second[index]->GetRefCount() == 0)
			{
				m_keyTable.erase(iterListFind->second[index]->GetSrcFilePath());
				m_indexTable.erase(indexTableUid);
				iterListFind->second[index]->Unload();
				delete iterListFind->second[index];
				iterListFind->second.erase(iterListFind->second.begin() + index);
				RefreshIndexTable();
			}
		}
	}
}

void ShaderContainer::RefreshIndexTable()
{	
	auto iterFind = m_indexTable.end();
	for (auto& curList : m_shaderList)
	{
		for (int i = 0; i < curList.second.size(); i++)
		{
			iterFind = m_indexTable.find(curList.second[i]->GetUID());
			if (iterFind != m_indexTable.end())
			{
				iterFind->second = i;
			}
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
	auto iterIdxFind = m_indexTable.find(key);
	if (iterIdxFind != m_indexTable.end())
	{
		if (iterIdxFind->second < m_hitGroupList.size())
		{
			hitGroup = &m_hitGroupList[iterIdxFind->second];
		}
	}

	if (hitGroup == nullptr)
	{
		m_hitGroupList.emplace_back();
		uint32_t addedIndex = static_cast<uint32_t>(m_hitGroupList.size()) - 1;
		m_hitGroupList[addedIndex].IncRef();
		if (!closetHitFilePath.empty())
		{
			m_hitGroupList[addedIndex].LoadShader(SHADER_TYPE_CLOSET_HIT, closetHitFilePath);
		}
		if (!anyHitFilePath.empty())
		{
			m_hitGroupList[addedIndex].LoadShader(SHADER_TYPE_ANY_HIT, anyHitFilePath);
		}
		if (!intersectionFilePath.empty())
		{
			m_hitGroupList[addedIndex].LoadShader(SHADER_TYPE_INTERSECTION, intersectionFilePath);
		}
		hitGroup = &m_hitGroupList[addedIndex];
		m_indexTable.insert(std::make_pair(key, addedIndex));
	}
	return hitGroup;
}

void RayHitGroupContainer::RemoveUnusedShaderGroups()
{
	std::vector<RtHitShaderGroup*> m_removeList;
	for (auto& cur : m_hitGroupList)
	{
		if (cur.GetRefCount() == 0)
		{
			m_removeList.push_back(&cur);
		}
	}

	for (auto cur : m_removeList)
	{
		HitGroupKey key = GetKeyFromGroup(cur);
		auto iterFind = m_indexTable.find(key);
		if (iterFind != m_indexTable.end())
		{
			m_indexTable.erase(key);
		}
	}

	RefreshIndexTable();
}

void RayHitGroupContainer::Clear()
{
	for (auto& cur : m_hitGroupList)
	{
		if (cur.GetRefCount() == 0)
		{
			cur.Destroy();
		}
	}
}

int RayHitGroupContainer::GetBindIndex(RtHitShaderGroup* hitShaderGroup)
{
	HitGroupKey key = GetKeyFromGroup(hitShaderGroup);
	auto iterFind = m_indexTable.find(key);
	if (iterFind != m_indexTable.end())
	{
		return iterFind->second;
	}
	return INVALID_INDEX_INT;
}

RtHitShaderGroup* RayHitGroupContainer::GetHitGroup(int index)
{
	if (index < m_hitGroupList.size())
	{
		return &m_hitGroupList[index];
	}
	return nullptr;
}

void RayHitGroupContainer::RefreshIndexTable()
{
	auto iterFind = m_indexTable.end();
	for (uint32_t i = 0; i < m_hitGroupList.size(); i++)
	{
		HitGroupKey key = GetKeyFromGroup(&m_hitGroupList[i]);
		iterFind = m_indexTable.find(key);
		if (iterFind != m_indexTable.end())
		{
			iterFind->second = i;
		}
	}
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
