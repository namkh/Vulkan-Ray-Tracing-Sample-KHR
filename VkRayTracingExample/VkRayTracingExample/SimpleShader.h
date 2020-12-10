#pragma once

#include "VulkanDeviceResources.h"
#include "Utils.h"

enum ERTShaderType
{
	SHADER_TYPE_INVALID = -1,
	SHADER_TYPE_RAY_GEN = 0,
	SHADER_TYPE_MISS,
	SHADER_TYPE_CALLABLE,
	SHADER_TYPE_INTERSECTION,
	SHADER_TYPE_ANY_HIT,
	SHADER_TYPE_CLOSET_HIT,
	SHADER_TYPE_END,
};

enum ERTShaderGroupType
{
	SHADER_GROUP_TYPE_INVALID = -1,
	SHADER_GROUP_TYPE_RAY_GEN = 0,
	SHADER_GROUP_TYPE_MISS,
	SHADER_GROUP_TYPE_HIT,
	SHADER_GROUP_TYPE_CALLABLE,
	SHADER_GROUP_TYPE_END
};

class SimpleShader : public RefCounter, public UniqueIdentifier
{
friend class ShaderContainer;
public:
	void Destroy();

	VkShaderModule& GetShaderModule() { return m_shaderModule; }
	std::string GetSrcFilePath() { return m_srcFilePath; }
	ERTShaderType GetShaderType() { return m_shaderType; }

protected:
	bool Load(ERTShaderType shaderType, std::string& shaderFilePath);
	void Unload();

private:
	ERTShaderType m_shaderType = SHADER_TYPE_INVALID;
	VkShaderModule m_shaderModule = VK_NULL_HANDLE;

	std::string m_srcFilePath = "";
};

class RtHitShaderGroup : public RefCounter, public UniqueIdentifier
{
public:
	void LoadShader(ERTShaderType shaderType, std::string& shaderFilePath);
	void Destroy();

	SimpleShader* GetShader(ERTShaderType shaderType);
	uint32_t GetBindIndex(ERTShaderType shaderType);

	bool Compare(RtHitShaderGroup& right);
	
private:
	std::map<ERTShaderType, SimpleShader*> m_hitShaders = {}; 
};

