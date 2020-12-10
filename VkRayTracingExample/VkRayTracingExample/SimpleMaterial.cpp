#include "SimpleMaterial.h"

void SimpleMaterial::Destory()
{
	if (m_diffuseTex != nullptr)
	{
		m_diffuseTex->Destroy();
	}
	if (m_normalTex != nullptr)
	{
		m_normalTex->Destroy();
	}
	if (m_roughnessTex != nullptr)
	{
		m_roughnessTex->Destroy();
	}
	if (m_metallicTex != nullptr)
	{
		m_metallicTex->Destroy();
	}
	if (m_ambientOcclusionTex != nullptr)
	{
		m_ambientOcclusionTex->Destroy();
	}
	if (m_hitShaderGroup != nullptr)
	{
		m_hitShaderGroup->Destroy();
	}
}