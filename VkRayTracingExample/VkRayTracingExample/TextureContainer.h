#pragma once

#include "SimpleTexture.h"
#include "Singleton.h"
#include "Utils.h"

class TextureContainer : public TSingleton<TextureContainer>
{
public:
	TextureContainer(token) 
	{
		m_textureList.reserve(RESOURCE_CONTAINER_INITIAL_SIZE);
	};
public:
	SimpleTexture2D* CreateTexture(const char* filePath);
	void RemoveUnusedTextrures();
	void Clear();
	
	int GetBindIndex(SimpleTexture2D* texture);
	uint32_t GetTextureCount() { return static_cast<uint32_t>(m_textureList.size()); }
	SimpleTexture2D* GetTexture(uint32_t  index)
	{
		if (index < m_textureList.size())
		{
			return m_textureList[index];
		}
		return nullptr;
	}

protected:
	SimpleTexture2D* LoadTexture(const char* filePath);
	void UnloadTexture(SimpleTexture2D* texture);
	
	void RefreshIndexTable();
	
private:

	std::map<std::string, UID> m_keyTable;
	std::map<UID, uint32_t> m_indexTable;
	std::vector<SimpleTexture2D*> m_textureList;
};

#define gTexContainer TextureContainer::Instance()