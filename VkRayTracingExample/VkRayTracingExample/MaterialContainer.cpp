#include "MaterialContainer.h"
#include "ShaderContainer.h"
#include "TextureContainer.h"

SimpleMaterial* MaterialContainer::CreateMaterial(ExampleMaterialType matType)
{
	SimpleMaterial* material = nullptr;

	auto iterUidFind = m_materialUidTable.find(matType);
	if (iterUidFind != m_materialUidTable.end())
	{
		auto iterFind = m_materialDatas.find(iterUidFind->second);
		if (iterFind != m_materialDatas.end())
		{
			material = iterFind->second;
		}
	}

	if (material == nullptr)
	{
		material = new SimpleMaterial();
		UID uid = material->GetUID();
		ExampleMaterialCreateCode::Instance().CreateExampleMaterial(matType, material);
		m_materialDatas.insert(std::make_pair(uid, material));
		m_materialUidTable.insert(std::make_pair(matType, uid));
	}

	return material;
}

void MaterialContainer::RemoveMaterial(SimpleMaterial* material)
{
	auto iterFind = m_materialDatas.find(material->GetUID());
	if (iterFind != m_materialDatas.end())
	{
		if (iterFind->second != nullptr)
		{
			m_materialUidTable.erase(iterFind->second->m_materialType);
			iterFind->second->Destory();
			delete iterFind->second;
		}
		m_materialDatas.erase(iterFind);
	}
}

int MaterialContainer::GetBindIndex(SimpleMaterial* material)
{
	auto iterFind = m_materialDatas.find(material->GetUID());
	if (iterFind != m_materialDatas.end())
	{
		return static_cast<int>(std::distance(m_materialDatas.begin(), iterFind));
	}
	return INVALID_INDEX_INT;
}

void MaterialContainer::Clear()
{
	for (auto& cur : m_materialDatas)
	{
		if (cur.second != nullptr)
		{
			cur.second->Destory();
		}
		delete cur.second;
	}

	m_materialDatas.clear();
	m_materialUidTable.clear();
}

SimpleMaterial* MaterialContainer::GetMaterial(int index)
{
	if (index < m_materialDatas.size())
	{
		auto pos = m_materialDatas.begin();
		std::advance(pos, index);
		return pos->second;
	}
	return nullptr;
}

//머트리얼 하드코딩 작성
void ExampleMaterialCreateCode::CreateExampleMaterial(ExampleMaterialType type, SimpleMaterial* mat)
{
	static std::string COMMON_CLOSET_HIT_SHADER_PATH = "../Resources/Shaders/Hit.spr";
	static std::string DEFAULT_CLOSET_HIT_SHADER_PATH = "../Resources/Shaders/Hit_Default.spr";
	static std::string TRANSPARENT_CLOSET_HIT_SHADER_PATH = "../Resources/Shaders/Hit_Transparent.spr";
	static std::string REFRACT_CLOSET_HIT_SHADER_PATH = "../Resources/Shaders/Hit_Refract.spr";
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
			mat->m_hitShaderGroup = gHitGroupContainer.CreateHitGroup(DEFAULT_CLOSET_HIT_SHADER_PATH, DEFAULT_ANY_HIT_SHADER_PATH, DEFAULT_INTERSECTION_SHADER_PATH);
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
			mat->m_hitShaderGroup = gHitGroupContainer.CreateHitGroup(DEFAULT_CLOSET_HIT_SHADER_PATH, DEFAULT_ANY_HIT_SHADER_PATH, DEFAULT_INTERSECTION_SHADER_PATH);
			break;
		case ExampleMaterialType::EXAMPLE_MAT_TYPE_METAL3 :
			mat->m_mateiralTypeIndex = static_cast<uint32_t>(EMaterialType::SURFACE_TYPE_DEFAULT);
			mat->m_indexOfRefraction = IOR_IRON;
			mat->m_diffuseTex = gTexContainer.CreateTexture("../Resources/Textures/Metal3/metal3_basecolor.png");
			mat->m_normalTex = gTexContainer.CreateTexture("../Resources/Textures/Metal3/metal3_normal.png");
			mat->m_roughnessTex = gTexContainer.CreateTexture("../Resources/Textures/Metal3/metal3_roughness.png");
			mat->m_metallicTex = gTexContainer.CreateTexture("../Resources/Textures/Metal3/metal3_metallic.png");
			mat->m_ambientOcclusionTex = gTexContainer.CreateTexture("../Resources/Textures/Metal3/metal3_ao.png");
			mat->m_hitShaderGroup = gHitGroupContainer.CreateHitGroup(DEFAULT_CLOSET_HIT_SHADER_PATH, DEFAULT_ANY_HIT_SHADER_PATH, DEFAULT_INTERSECTION_SHADER_PATH);
			break;
		case ExampleMaterialType::EXAMPLE_MAT_TYPE_GLASS:
			mat->m_mateiralTypeIndex = static_cast<uint32_t>(EMaterialType::MATERIAL_TYPE_TRANSPARENT_REFRACT);
			mat->m_indexOfRefraction = IOR_GLASS;
			mat->m_diffuseTex = gTexContainer.CreateTexture("../Resources/Textures/Glass/glass_basecolor.png");
			mat->m_normalTex = gTexContainer.CreateTexture("../Resources/Textures/Glass/glass_normal.png");
			mat->m_roughnessTex = gTexContainer.CreateTexture("../Resources/Textures/Glass/glass_roughness.png");
			mat->m_metallicTex = gTexContainer.CreateTexture("../Resources/Textures/Glass/glass_metallic.png");
			mat->m_hitShaderGroup = gHitGroupContainer.CreateHitGroup(REFRACT_CLOSET_HIT_SHADER_PATH, DEFAULT_ANY_HIT_SHADER_PATH, DEFAULT_INTERSECTION_SHADER_PATH);
			break;
		case ExampleMaterialType::EXAMPLE_MAT_TYPE_PAINT_TRANSPARENT:
			mat->m_mateiralTypeIndex = static_cast<uint32_t>(EMaterialType::SURFACE_TYPE_TRANSPARENT);
			mat->m_indexOfRefraction = IOR_IRON;
			mat->m_color = glm::vec4(1.0f, 1.0f, 1.0f, 0.3f);
			mat->m_diffuseTex = gTexContainer.CreateTexture("../Resources/Textures/Paint/Paint_basecolor.png");
			mat->m_normalTex = gTexContainer.CreateTexture("../Resources/Textures/Paint/Paint_normal.png");
			mat->m_roughnessTex = gTexContainer.CreateTexture("../Resources/Textures/Paint/Paint_roughness.png");
			mat->m_metallicTex = gTexContainer.CreateTexture("../Resources/Textures/Paint/Paint_metallic.png");
			mat->m_hitShaderGroup = gHitGroupContainer.CreateHitGroup(TRANSPARENT_CLOSET_HIT_SHADER_PATH, DEFAULT_ANY_HIT_SHADER_PATH, DEFAULT_INTERSECTION_SHADER_PATH);
			break;

		case ExampleMaterialType::EXAMPLE_MAT_TYPE_PLATE:
			mat->m_mateiralTypeIndex = static_cast<uint32_t>(EMaterialType::SURFACE_TYPE_DEFAULT);
			mat->m_indexOfRefraction = IOR_IRON;
			mat->m_diffuseTex = gTexContainer.CreateTexture("../Resources/Textures/Plate/Plate_basecolor.png");
			mat->m_normalTex = gTexContainer.CreateTexture("../Resources/Textures/Plate/Plate_normal.png");
			mat->m_roughnessTex = gTexContainer.CreateTexture("../Resources/Textures/Plate/Plate_roughness.png");
			mat->m_metallicTex = gTexContainer.CreateTexture("../Resources/Textures/Plate/Plate_metallic.png");
			mat->m_ambientOcclusionTex = gTexContainer.CreateTexture("../Resources/Textures/Plate/Plate_ao.png");
			mat->m_hitShaderGroup = gHitGroupContainer.CreateHitGroup(DEFAULT_CLOSET_HIT_SHADER_PATH, DEFAULT_ANY_HIT_SHADER_PATH, DEFAULT_INTERSECTION_SHADER_PATH);
			mat->m_uvScale = 4.0f;
			break;
	}
	
}