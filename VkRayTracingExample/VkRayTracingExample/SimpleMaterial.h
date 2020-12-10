#pragma once

#include "SimpleTexture.h"
#include "SimpleShader.h"
#include "Utils.h"

const float IOR_AIR = 1.0003f;
const float IOR_GLASS = 1.52f;
const float IOR_PLASTIC = 1.460f;
const float IOR_IRON = 4.24f;

enum class EMaterialType
{
	SURFACE_TYPE_DEFAULT = 0,
	SURFACE_TYPE_TRANSPARENT,
	MATERIAL_TYPE_TRANSPARENT_REFRACT
};

enum class ExampleMaterialType
{
	EXAMPLE_MAT_TYPE_INVALID,
	EXAMPLE_MAT_TYPE_METAL1,
	EXAMPLE_MAT_TYPE_METAL2,
	EXAMPLE_MAT_TYPE_METAL3,
	EXAMPLE_MAT_TYPE_GLASS,
	EXAMPLE_MAT_TYPE_PAINT_TRANSPARENT,
	EXAMPLE_MAT_TYPE_PLATE,
};

class SimpleMaterialInstance
{
public:
	

};

//에디터가 없으므로 적당히 작성
class SimpleMaterial : public UniqueIdentifier
{
public:

	void Destory();

	//일단은 이 이데이터들만 사용//
	/*이데이터들 인스턴스 화 해야한다..*/
	glm::vec4	m_color = glm::vec4(1.0f);
	uint32_t	m_mateiralTypeIndex = 0;
	float		m_indexOfRefraction = 1.0f;
	float		m_metallic = 0.0f;
	float		m_roughness = 1.0f;

	//hit shader offset 같은걸 둬야할까??
	//일단 재질 단위로 나누고 그렇게 바꾸자

	SimpleTexture2D* m_diffuseTex = nullptr;
	SimpleTexture2D* m_normalTex = nullptr;
	SimpleTexture2D* m_roughnessTex = nullptr;
	SimpleTexture2D* m_metallicTex = nullptr;
	SimpleTexture2D* m_ambientOcclusionTex = nullptr;

	float m_uvScale = 1.0f;
	
	ExampleMaterialType m_materialType = ExampleMaterialType::EXAMPLE_MAT_TYPE_INVALID;
	RtHitShaderGroup* m_hitShaderGroup;
};