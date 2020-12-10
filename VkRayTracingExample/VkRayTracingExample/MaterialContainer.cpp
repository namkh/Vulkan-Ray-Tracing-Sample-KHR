#include "MaterialContainer.h"
#include "ShaderContainer.h"
#include "TextureContainer.h"

SimpleMaterial* MaterialContainer::CreateMaterial(ExampleMaterialType matType)
{
	SimpleMaterial* material = new SimpleMaterial();
	m_materialList.push_back(material);

	ExampleMaterialCreateCode::Instance().CreateExampleMaterial(matType, material);
	UID uid = material->GetUID();
	m_materialIndexTable.insert(std::make_pair(uid, static_cast<uint32_t>(m_materialList.size() - 1)));

	return material;
}

void MaterialContainer::RemoveMaterial(SimpleMaterial* material)
{
	auto iterIdxFind = m_materialIndexTable.find(material->GetUID());
	if (iterIdxFind != m_materialIndexTable.end())
	{
		int findIndex = iterIdxFind->second;
		m_materialIndexTable.erase(iterIdxFind);
		m_materialList[findIndex]->Destory();
		delete m_materialList[findIndex];
		m_materialList.erase(m_materialList.begin() + findIndex);

		RefreshIndexTable();
	}
}

int MaterialContainer::GetBindIndex(SimpleMaterial* material)
{
	auto iterFind = m_materialIndexTable.find(material->GetUID());
	if (iterFind != m_materialIndexTable.end())
	{
		return iterFind->second;
	}
	return INVALID_INDEX_INT;
}

void MaterialContainer::Clear()
{
	for (auto& cur : m_materialList)
	{
		if (cur != nullptr)
		{
			cur->Destory();
		}
		delete cur;
	}

	m_materialList.clear();
	m_materialIndexTable.clear();
}

SimpleMaterial* MaterialContainer::GetMaterial(int index)
{
	if (index < m_materialList.size())
	{
		return m_materialList[index];
	}
	return nullptr;
}

void MaterialContainer::RefreshIndexTable()
{
	auto iterFind = m_materialIndexTable.end();
	for (int i = 0; i < m_materialList.size(); i++)
	{
		iterFind = m_materialIndexTable.find(m_materialList[i]->GetUID());
		if (iterFind != m_materialIndexTable.end())
		{
			iterFind->second = i;
		}
	}
}

//머트리얼 하드코딩 작성
void ExampleMaterialCreateCode::CreateExampleMaterial(ExampleMaterialType type, SimpleMaterial* mat)
{
	static std::string DEFAULT_CLOSET_HIT_SHADER_PATH = "../Resources/Shaders/Hit.spr";
	static std::string DEFAULT_ANY_HIT_SHADER_PATH = "";
	static std::string DEFAULT_INTERSECTION_SHADER_PATH = "";

	mat->m_materialType = type;
	switch (type)
	{
		case ExampleMaterialType::EXAMPLE_MAT_TYPE_METAL1 :
			mat->m_color = glm::vec4(0.56f, 0.57f, 0.58f, 1.0f);
			mat->m_mateiralTypeIndex = static_cast<uint32_t>(EMaterialType::SURFACE_TYPE_DEFAULT);
			mat->m_indexOfRefraction = IOR_IRON;
			mat->m_diffuseTex = gTexContainer.CreateTexture("../Resources/Textures/Metal1/metal1_basecolor.png");
			mat->m_normalTex = gTexContainer.CreateTexture("../Resources/Textures/Metal1/metal1_normal.png");
			mat->m_roughnessTex = gTexContainer.CreateTexture("../Resources/Textures/Metal1/metal1_roughness.png");
			mat->m_metallicTex = gTexContainer.CreateTexture("../Resources/Textures/Metal1/metal1_metallic.png");
			mat->m_ambientOcclusionTex = gTexContainer.CreateTexture("../Resources/Textures/Metal1/metal1_ao.png");
			break;
		case ExampleMaterialType::EXAMPLE_MAT_TYPE_METAL2 :
			mat->m_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			mat->m_mateiralTypeIndex = static_cast<uint32_t>(EMaterialType::SURFACE_TYPE_DEFAULT);
			mat->m_indexOfRefraction = IOR_IRON;
			mat->m_diffuseTex = gTexContainer.CreateTexture("../Resources/Textures/Metal2/metal2_basecolor.png");
			mat->m_normalTex = gTexContainer.CreateTexture("../Resources/Textures/Metal2/metal2_normal.png");
			mat->m_roughnessTex = gTexContainer.CreateTexture("../Resources/Textures/Metal2/metal2_roughness.png");
			mat->m_metallicTex = gTexContainer.CreateTexture("../Resources/Textures/Metal2/metal2_metallic.png");
			mat->m_ambientOcclusionTex = gTexContainer.CreateTexture("../Resources/Textures/Metal2/metal2_ao.png");
			break;
		case ExampleMaterialType::EXAMPLE_MAT_TYPE_METAL3 :
			mat->m_mateiralTypeIndex = static_cast<uint32_t>(EMaterialType::SURFACE_TYPE_DEFAULT);
			mat->m_indexOfRefraction = IOR_IRON;
			mat->m_diffuseTex = gTexContainer.CreateTexture("../Resources/Textures/Metal3/metal3_basecolor.png");
			mat->m_normalTex = gTexContainer.CreateTexture("../Resources/Textures/Metal3/metal3_normal.png");
			mat->m_roughnessTex = gTexContainer.CreateTexture("../Resources/Textures/Metal3/metal3_roughness.png");
			mat->m_metallicTex = gTexContainer.CreateTexture("../Resources/Textures/Metal3/metal3_metallic.png");
			mat->m_ambientOcclusionTex = gTexContainer.CreateTexture("../Resources/Textures/Metal3/metal3_ao.png");
			break;
		case ExampleMaterialType::EXAMPLE_MAT_TYPE_GLASS:
			mat->m_mateiralTypeIndex = static_cast<uint32_t>(EMaterialType::MATERIAL_TYPE_TRANSPARENT_REFRACT);
			mat->m_indexOfRefraction = IOR_GLASS;
			mat->m_diffuseTex = gTexContainer.CreateTexture("../Resources/Textures/Glass/glass_basecolor.png");
			mat->m_normalTex = gTexContainer.CreateTexture("../Resources/Textures/Glass/glass_normal.png");
			mat->m_roughnessTex = gTexContainer.CreateTexture("../Resources/Textures/Glass/glass_roughness.png");
			mat->m_metallicTex = gTexContainer.CreateTexture("../Resources/Textures/Glass/glass_metallic.png");
			break;
		case ExampleMaterialType::EXAMPLE_MAT_TYPE_PAINT_TRANSPARENT:
			mat->m_mateiralTypeIndex = static_cast<uint32_t>(EMaterialType::SURFACE_TYPE_TRANSPARENT);
			mat->m_indexOfRefraction = IOR_IRON;
			mat->m_color = glm::vec4(1.0f, 1.0f, 1.0f, 0.6f);
			mat->m_diffuseTex = gTexContainer.CreateTexture("../Resources/Textures/Paint/Paint_basecolor.png");
			mat->m_normalTex = gTexContainer.CreateTexture("../Resources/Textures/Paint/Paint_normal.png");
			mat->m_roughnessTex = gTexContainer.CreateTexture("../Resources/Textures/Paint/Paint_roughness.png");
			mat->m_metallicTex = gTexContainer.CreateTexture("../Resources/Textures/Paint/Paint_metallic.png");
			break;

		case ExampleMaterialType::EXAMPLE_MAT_TYPE_PLATE:
			mat->m_mateiralTypeIndex = static_cast<uint32_t>(EMaterialType::SURFACE_TYPE_DEFAULT);
			mat->m_indexOfRefraction = IOR_IRON;
			mat->m_diffuseTex = gTexContainer.CreateTexture("../Resources/Textures/Plate/Plate_basecolor.png");
			mat->m_normalTex = gTexContainer.CreateTexture("../Resources/Textures/Plate/Plate_normal.png");
			mat->m_roughnessTex = gTexContainer.CreateTexture("../Resources/Textures/Plate/Plate_roughness.png");
			mat->m_metallicTex = gTexContainer.CreateTexture("../Resources/Textures/Plate/Plate_metallic.png");
			mat->m_ambientOcclusionTex = gTexContainer.CreateTexture("../Resources/Textures/Plate/Plate_ao.png");
			mat->m_uvScale = 4.0f;
			break;
	}
	mat->m_hitShaderGroup = gHitGroupContainer.CreateHitGroup(DEFAULT_CLOSET_HIT_SHADER_PATH, DEFAULT_ANY_HIT_SHADER_PATH, DEFAULT_INTERSECTION_SHADER_PATH);
}