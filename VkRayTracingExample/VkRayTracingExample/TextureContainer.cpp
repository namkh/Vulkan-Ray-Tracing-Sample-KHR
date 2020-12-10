#include "TextureContainer.h"

SimpleTexture2D* TextureContainer::CreateTexture(const char* filePath)
{
	SimpleTexture2D* texture = nullptr;

	//로드 되어있는지 검색
	auto iterKeyFinded = m_keyTable.find(filePath);
	if (iterKeyFinded != m_keyTable.end())
	{
		auto iterIndexFinded = m_indexTable.find(iterKeyFinded->second);
		if (iterIndexFinded != m_indexTable.end() && iterIndexFinded->second < m_textureList.size())
		{
			texture = m_textureList[iterIndexFinded->second];
		}
	}

	//로드 되어있지 않다면 로드한다
	if (texture == nullptr)
	{
		texture = LoadTexture(filePath);
		if (texture == nullptr)
		{
			//로딩 실패 로깅....
		}
	}

	if (texture != nullptr)
	{
		texture->IncRef();
	}

	return texture;
}

int TextureContainer::GetBindIndex(SimpleTexture2D* texture)
{
	auto iterFind = m_indexTable.find(texture->GetUID());
	if (iterFind != m_indexTable.end())
	{
		return iterFind->second;
	}
	return INVALID_INDEX_INT;
}

void TextureContainer::RemoveUnusedTextrures()
{
	std::vector<SimpleTexture2D*> m_removeList;
	for (auto& cur : m_textureList)
	{
		if (cur->GetRefCount() == 0)
		{
			m_removeList.push_back(cur);
		}
	}

	for (auto cur : m_removeList)
	{
		UnloadTexture(cur);
	}
}

SimpleTexture2D* TextureContainer::LoadTexture(const char* filePath)
{
	SimpleTexture2D* texture = new SimpleTexture2D();
	m_textureList.push_back(texture);

	if (!texture->Load(filePath))
	{
		//텍스쳐 로딩실패 로깅
		return nullptr;
	}

	UID uid = texture->GetUID();
	m_keyTable.insert(std::make_pair(filePath, uid));
	m_indexTable.insert(std::make_pair(uid, static_cast<uint32_t>(m_textureList.size() - 1)));

	return texture;
}

void TextureContainer::UnloadTexture(SimpleTexture2D* texture)
{
	UID indexTableUid = texture->GetUID();
	auto iterIdxFind = m_indexTable.find(indexTableUid);
	if (iterIdxFind != m_indexTable.end())
	{
		uint32_t index = iterIdxFind->second;
		if (index < m_textureList.size() && m_textureList[index]->GetRefCount() == 0)
		{
			m_keyTable.erase(m_textureList[iterIdxFind->second]->GetSrcFilePath());
			m_indexTable.erase(indexTableUid);
			m_textureList[index]->Unload();
			delete m_textureList[index];
			m_textureList.erase(m_textureList.begin() + index);
		}
	}
}

void TextureContainer::Clear()
{
	for (auto& cur : m_textureList)
	{
		if (cur != nullptr && cur->GetRefCount() == 0)
		{
			cur->Unload();
			delete cur;
		}
		else
		{
			continue;
		}
	}
}

void TextureContainer::RefreshIndexTable()
{
	auto iterFind = m_indexTable.end();
	for (int i = 0; i < m_textureList.size(); i++)
	{
		iterFind = m_indexTable.find(m_textureList[i]->GetUID());
		if (iterFind != m_indexTable.end())
		{
			iterFind->second = i;
		}
	}
}