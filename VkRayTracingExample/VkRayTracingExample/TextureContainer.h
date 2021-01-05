#pragma once

#include "SimpleTexture.h"
#include "Singleton.h"
#include "Utils.h"

#include <unordered_map>

class TextureContainer : public TSingleton<TextureContainer>
{
public:
	TextureContainer(token) 
	{
	};
public:
	SimpleTexture2D* CreateTexture(const char* filePath);
	void RemoveUnusedTextrures();
	void Clear();
	
	int GetBindIndex(SimpleTexture2D* texture);
	uint32_t GetTextureCount() { return static_cast<uint32_t>(m_textureDatas.size()); }
	SimpleTexture2D* GetTexture(uint32_t  index);

protected:
	SimpleTexture2D* LoadTexture(const char* filePath);
	void UnloadTexture(SimpleTexture2D* texture);
	
private:
	std::map<std::string, UID> m_keyTable;
	std::unordered_map<UID, SimpleTexture2D*> m_textureDatas;
};

#define gTexContainer TextureContainer::Instance()