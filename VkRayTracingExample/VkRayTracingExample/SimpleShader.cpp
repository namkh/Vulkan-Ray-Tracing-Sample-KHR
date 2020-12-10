#include "SimpleShader.h"
#include "ShaderContainer.h"

void SimpleShader::Destroy()
{
	DecRef();
}

bool SimpleShader::Load(ERTShaderType shaderType, std::string& shaderFilePath)
{
	m_shaderType = shaderType;
	m_srcFilePath = shaderFilePath;
	std::ifstream shaderFile(shaderFilePath, std::ios::binary);
	if (shaderFile.is_open())
	{
		shaderFile.seekg(0, std::ios_base::end);
		uint32_t shaderFileSize = static_cast<uint32_t>(shaderFile.tellg());

		if (shaderFileSize > 0)
		{
			std::vector<char> binaryData(shaderFileSize);
			shaderFile.seekg(0, std::ios_base::beg);
			shaderFile.read(binaryData.data(), shaderFileSize);

			VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
			shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shaderModuleCreateInfo.codeSize = shaderFileSize;
			shaderModuleCreateInfo.pCode = reinterpret_cast<std::uint32_t const*>(binaryData.data());

			VkResult res = vkCreateShaderModule(gLogicalDevice, &shaderModuleCreateInfo, nullptr, &m_shaderModule);
			if (res == VkResult::VK_SUCCESS)
			{
				return true;
			}
		}
	}
	return false;
}

void SimpleShader::Unload()
{
	if (m_shaderModule != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(gLogicalDevice, m_shaderModule, nullptr);
	}
}

//path를 통해 로드하도록 바꾸자
void RtHitShaderGroup::LoadShader(ERTShaderType shaderType, std::string& shaderFilePath)
{
	auto iterFind = m_hitShaders.find(shaderType);
	if(iterFind != m_hitShaders.end())
	{
		iterFind->second->Destroy();
		m_hitShaders.erase(iterFind);
	}
	
	SimpleShader* shader = gShaderContainer.CreateShader(shaderType, shaderFilePath);
	if (shader)
	{
		m_hitShaders.insert(std::make_pair(shaderType, shader));
	}
}

void RtHitShaderGroup::Destroy()
{
	DecRef();

	for (auto& cur : m_hitShaders)
	{
		cur.second->Destroy();
	}
}

SimpleShader* RtHitShaderGroup::GetShader(ERTShaderType shaderType)
{
	auto iterFind = m_hitShaders.find(shaderType);
	if (iterFind != m_hitShaders.end())
	{
		return iterFind->second;
	}
	return nullptr;
}

uint32_t RtHitShaderGroup::GetBindIndex(ERTShaderType shaderType)
{
	uint32_t index = VK_SHADER_UNUSED_KHR;
	auto iterFind = m_hitShaders.find(shaderType);
	if (iterFind != m_hitShaders.end())
	{
		index = gShaderContainer.GetBindIndex(iterFind->second);
	}
	return index;
}

bool RtHitShaderGroup::Compare(RtHitShaderGroup& right)
{
	return 
	GetBindIndex(SHADER_TYPE_ANY_HIT) == right.GetBindIndex(SHADER_TYPE_ANY_HIT) &&
	GetBindIndex(SHADER_TYPE_INTERSECTION) == right.GetBindIndex(SHADER_TYPE_INTERSECTION) &&
	GetBindIndex(SHADER_TYPE_CLOSET_HIT) == right.GetBindIndex(SHADER_TYPE_CLOSET_HIT);
}
