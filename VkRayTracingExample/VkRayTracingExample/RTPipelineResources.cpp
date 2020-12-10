#include "RTPipelineResources.h"

#include "RenderObjectContainer.h"
#include "MaterialContainer.h"
#include "GeometryContainer.h"


bool RTPipelineResources::Build(VkAccelerationStructureKHR tlasHandle, RtTargetImageBuffer* targetImageBuffer, SimpleCubmapTexture* cubeMap)
{
	if (!CreateBuffers())
	{
		//assert가 편하긴한데... message + assert or exception
		return false;
	}

	if (!CreateDescriptorPool())
	{
		return false;
	}

	if (!RefreshResourceBind(tlasHandle, targetImageBuffer, cubeMap))
	{
		return false;
	}
	
	return true;
}

bool RTPipelineResources::RefreshResourceBind(VkAccelerationStructureKHR tlasHandle, RtTargetImageBuffer* targetImageBuffer, SimpleCubmapTexture* cubeMap)
{
	if (m_tlasHandle != VK_NULL_HANDLE)
	{
		DestoryBindLayouts();
	}

	m_tlasHandle = tlasHandle;
	m_targetImageBuffer = targetImageBuffer;
	m_skyCubeMap = cubeMap;

	if (!RefreshResourceBind())
	{
		return false;
	}
	RefreshWriteDescriptorSet();

	return true;
}

void RTPipelineResources::Update(GlobalConstants& globalConstnats, VkAccelerationStructureKHR tlasHandle)
{
	//업데이트 마크 만들고 갱신된 인덱스만 수집하자
	UpdateInstanceConstants();
	UpdateMaterialConstants();
	UpdateGlobalConstants(globalConstnats);

	if (tlasHandle != VK_NULL_HANDLE)
	{
		RefreshResourceBind(tlasHandle, m_targetImageBuffer, m_skyCubeMap);
	}
}

void RTPipelineResources::Destroy()
{
	m_instanceConstantsBuffer.Destroy();
	m_materialConstantsBuffer.Destroy();
	m_globalConstantsBuffer.Destroy();

	DestoryBindLayouts();

	if (m_descPool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(gLogicalDevice, m_descPool, nullptr);
	}
}

bool RTPipelineResources::CreateBuffers()
{
	m_instanceConstants.resize(gRenderObjContainer.GetRenderObjectInstancePerMeshCount());
	bool res = m_instanceConstantsBuffer.Initialzie
	(
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		static_cast<uint32_t>(m_instanceConstants.size())
	);
	if (!res)
	{
		//인스턴스 버퍼생성 실패를 로깅
		return false;
	}

	m_materialConstants.resize(gMaterialContainer.GetMaterialCount());
	res = m_materialConstantsBuffer.Initialzie
	(
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		static_cast<uint32_t>(m_materialConstants.size())
	);
	if (!res)
	{
		//머트리얼 버퍼생성 실패를 로깅
		return false;
	}

	res = m_globalConstantsBuffer.Initialzie
	(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);
	if (!res)
	{
		//globalConstant 버퍼생성 실패를 로깅
		return false;
	}
	return res;
}

bool RTPipelineResources::CreateDescriptorPool()
{
	std::vector<VkDescriptorPoolSize> descPoolSize(4);
	descPoolSize[0].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	descPoolSize[0].descriptorCount = 1;
	descPoolSize[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descPoolSize[1].descriptorCount = 1;
	descPoolSize[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descPoolSize[2].descriptorCount = 4; // instance, material, vb, ib
	descPoolSize[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descPoolSize[3].descriptorCount = 2;

	VkDescriptorPoolCreateInfo descPoolCreateInfo = {};
	descPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descPoolCreateInfo.maxSets = 1;
	descPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(descPoolSize.size());
	descPoolCreateInfo.pPoolSizes = descPoolSize.data();

	VkResult res = vkCreateDescriptorPool(gLogicalDevice, &descPoolCreateInfo, nullptr, &m_descPool);
	if (res != VkResult::VK_SUCCESS)
	{
		//상수버퍼 디스크립터 풀 생성실패 로깅
		return false;
	}

	return true;
}

void RTPipelineResources::DestoryBindLayouts()
{
	if (m_pipelineLayout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(gLogicalDevice, m_pipelineLayout, nullptr);
		m_pipelineLayout = VK_NULL_HANDLE;
	}

	vkResetDescriptorPool(gLogicalDevice, m_descPool, 0);

	m_descSets.clear();
	
	for (auto& cur : m_descLayouts)
	{
		vkDestroyDescriptorSetLayout(gLogicalDevice, cur, nullptr);
	}
	m_descLayouts.clear();
}

bool RTPipelineResources::RefreshResourceBind()
{
	m_descSetLayoutBindings.resize(9);
	//enum을 둘까??
	//as
	m_descSetLayoutBindings[0].binding = 0;
	m_descSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	m_descSetLayoutBindings[0].descriptorCount = 1;
	m_descSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	//target image
	m_descSetLayoutBindings[1].binding = 1;
	m_descSetLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	m_descSetLayoutBindings[1].descriptorCount = 1;
	m_descSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

	//global constants
	m_descSetLayoutBindings[2].binding = 2;
	m_descSetLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	m_descSetLayoutBindings[2].descriptorCount = 1;
	m_descSetLayoutBindings[2].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	//instance constants
	m_descSetLayoutBindings[3].binding = 3;
	m_descSetLayoutBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	m_descSetLayoutBindings[3].descriptorCount = 1;
	m_descSetLayoutBindings[3].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	//material constants
	m_descSetLayoutBindings[4].binding = 4;
	m_descSetLayoutBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	m_descSetLayoutBindings[4].descriptorCount = 1;
	m_descSetLayoutBindings[4].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	//vb
	m_descSetLayoutBindings[5].binding = 5;
	m_descSetLayoutBindings[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	m_descSetLayoutBindings[5].descriptorCount = gGeomContainer.GetMeshCount();
	m_descSetLayoutBindings[5].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	//ib
	m_descSetLayoutBindings[6].binding = 6;
	m_descSetLayoutBindings[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	m_descSetLayoutBindings[6].descriptorCount = gGeomContainer.GetMeshCount();
	m_descSetLayoutBindings[6].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	//cube map
	m_descSetLayoutBindings[7].binding = 7;
	m_descSetLayoutBindings[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	m_descSetLayoutBindings[7].descriptorCount = 1;
	m_descSetLayoutBindings[7].stageFlags = VK_SHADER_STAGE_MISS_BIT_KHR;

	//texture array
	m_descSetLayoutBindings[8].binding = 8;
	m_descSetLayoutBindings[8].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	m_descSetLayoutBindings[8].descriptorCount = gTexContainer.GetTextureCount();
	m_descSetLayoutBindings[8].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = static_cast<uint32_t>(m_descSetLayoutBindings.size());
	layoutCreateInfo.pBindings = m_descSetLayoutBindings.data();

	
	m_descLayouts.resize(1);
	VkResult res = vkCreateDescriptorSetLayout(gLogicalDevice, &layoutCreateInfo, nullptr, m_descLayouts.data());
	if (res != VkResult::VK_SUCCESS)
	{
		//desc set 생성실패 로깅
		return false;
	}

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = m_descLayouts.data();
	
	res = vkCreatePipelineLayout(gLogicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout);
	if (res != VkResult::VK_SUCCESS)
	{
		//pipeline layout 생성 실패를 로깅
		return false;
	}

	std::vector<VkDescriptorSetAllocateInfo> descSetAllocateInfo(1);
	descSetAllocateInfo[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descSetAllocateInfo[0].pNext = nullptr;
	descSetAllocateInfo[0].descriptorPool = m_descPool;
	descSetAllocateInfo[0].descriptorSetCount = m_numDescriptorSet;
	descSetAllocateInfo[0].pSetLayouts = m_descLayouts.data();

	m_descSets.resize(m_numDescriptorSet);
	res = vkAllocateDescriptorSets(gLogicalDevice, descSetAllocateInfo.data(), m_descSets.data());
	if (res != VkResult::VK_SUCCESS)
	{
		//디스크립터 셋 생성실패 로깅
		return false;
	}

	return true;
}

void RTPipelineResources::RefreshWriteDescriptorSet()
{
	std::vector<VkDescriptorBufferInfo> vertexBufferInfos;
	std::vector<VkDescriptorBufferInfo> indexBufferInfos;
	for (uint32_t i = 0; i < gGeomContainer.GetMeshCount(); i++)
	{
		SimpleMeshData* curMeshData = gGeomContainer.GetMesh(i);
		vertexBufferInfos.push_back(curMeshData->GetVertexBuffer()->GetBufferInfo());
		indexBufferInfos.push_back(curMeshData->GetIndexBuffer()->GetBufferInfo());
	}

	std::vector<VkDescriptorImageInfo> imageInfos;
	for (uint32_t i = 0; i < gTexContainer.GetTextureCount(); i++)
	{
		SimpleTexture2D* texture = gTexContainer.GetTexture(i);
		if (texture != nullptr)
		{
			imageInfos.push_back(texture->GetImageInfo());
		}
	}

	VkWriteDescriptorSetAccelerationStructureKHR asWriteDesc = {};
	asWriteDesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	asWriteDesc.accelerationStructureCount = 1;
	asWriteDesc.pAccelerationStructures = &m_tlasHandle;

	//일단 이렇게 가고 나중에 구조화하자..
	std::vector<VkWriteDescriptorSet> writeDescs(9);
	writeDescs[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescs[0].pNext = &asWriteDesc;
	writeDescs[0].dstSet = m_descSets[0];
	writeDescs[0].dstBinding = 0;
	writeDescs[0].dstArrayElement = 0;
	writeDescs[0].descriptorCount = 1;
	writeDescs[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

	writeDescs[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescs[1].dstSet = m_descSets[0];
	writeDescs[1].dstBinding = 1;
	writeDescs[1].dstArrayElement = 0;
	writeDescs[1].descriptorCount = 1;
	writeDescs[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	writeDescs[1].pImageInfo = &m_targetImageBuffer->GetImageInfo();

	writeDescs[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescs[2].dstSet = m_descSets[0];
	writeDescs[2].dstBinding = 2;
	writeDescs[2].dstArrayElement = 0;
	writeDescs[2].descriptorCount = 1;
	writeDescs[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescs[2].pBufferInfo = &m_globalConstantsBuffer.GetBufferInfo();

	writeDescs[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescs[3].dstSet = m_descSets[0];
	writeDescs[3].dstBinding = 3;
	writeDescs[3].dstArrayElement = 0;
	writeDescs[3].descriptorCount = 1;
	writeDescs[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	writeDescs[3].pBufferInfo = &m_instanceConstantsBuffer.GetBufferInfo();

	writeDescs[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescs[4].dstSet = m_descSets[0];
	writeDescs[4].dstBinding = 4;
	writeDescs[4].dstArrayElement = 0;
	writeDescs[4].descriptorCount = 1;
	writeDescs[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	writeDescs[4].pBufferInfo = &m_materialConstantsBuffer.GetBufferInfo();

	writeDescs[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescs[5].dstSet = m_descSets[0];
	writeDescs[5].dstBinding = 5;
	writeDescs[5].dstArrayElement = 0;
	writeDescs[5].descriptorCount = static_cast<uint32_t>(indexBufferInfos.size());
	writeDescs[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	writeDescs[5].pBufferInfo = vertexBufferInfos.data();

	writeDescs[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescs[6].dstSet = m_descSets[0];
	writeDescs[6].dstBinding = 6;
	writeDescs[6].dstArrayElement = 0;
	writeDescs[6].descriptorCount = static_cast<uint32_t>(indexBufferInfos.size());
	writeDescs[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	writeDescs[6].pBufferInfo = indexBufferInfos.data();

	writeDescs[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescs[7].dstSet = m_descSets[0];
	writeDescs[7].dstBinding = 7;
	writeDescs[7].dstArrayElement = 0;
	writeDescs[7].descriptorCount = 1;
	writeDescs[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescs[7].pImageInfo = &m_skyCubeMap->GetImageInfo();

	writeDescs[8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescs[8].dstSet = m_descSets[0];
	writeDescs[8].dstBinding = 8;
	writeDescs[8].dstArrayElement = 0;
	writeDescs[8].descriptorCount = static_cast<uint32_t>(imageInfos.size());
	writeDescs[8].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescs[8].pImageInfo = imageInfos.data();

	vkUpdateDescriptorSets(gLogicalDevice, static_cast<uint32_t>(writeDescs.size()), writeDescs.data(), 0, nullptr);
}

//TODO :
//업데이트 된 데이터만 업로드 해주게 개선필요
void RTPipelineResources::UpdateInstanceConstants()
{
	uint32_t instPreMeshCount = gRenderObjContainer.GetRenderObjectInstancePerMeshCount();
	for (uint32_t i = 0; i < instPreMeshCount; i++)
	{
		SampleRenderObjectInstancePerMesh* instPerMesh = gRenderObjContainer.GetRenderObjectInstancePerMesh(i);

		if (instPerMesh != nullptr)
		{
			m_instanceConstants[i].GeometryID = gGeomContainer.GetMeshBindIndex(instPerMesh->GetMeshData());
			m_instanceConstants[i].MaterialID = gMaterialContainer.GetBindIndex(instPerMesh->GetMaterial());
			SampleRenderObjectInstance* parentInst = instPerMesh->GetParentInstance();
			if (parentInst != nullptr)
			{
				m_instanceConstants[i].WorldMat = parentInst->GetWorldMatrix();
			}
		}
	}

	m_instanceConstantsBuffer.UpdateResource(m_instanceConstants.data(), static_cast<uint32_t>(m_instanceConstants.size()));

}

void RTPipelineResources::UpdateMaterialConstants()
{
	uint32_t materialCount = gMaterialContainer.GetMaterialCount();
	for (uint32_t i = 0; i < materialCount; i++)
	{
		SimpleMaterial* curMaterial = gMaterialContainer.GetMaterial(i);
		if (curMaterial != nullptr)
		{
			m_materialConstants[i].m_color = curMaterial->m_color;
			m_materialConstants[i].MateiralTypeIndex = curMaterial->m_mateiralTypeIndex;
			m_materialConstants[i].IndexOfRefraction = curMaterial->m_indexOfRefraction;
			m_materialConstants[i].Metallic = curMaterial->m_metallic;
			m_materialConstants[i].Roughness = curMaterial->m_roughness;
			m_materialConstants[i].uvScale = curMaterial->m_uvScale;

			if (curMaterial->m_diffuseTex != nullptr)
			{
				m_materialConstants[i].DiffuseTexIndex = static_cast<int>(gTexContainer.GetBindIndex(curMaterial->m_diffuseTex));
			}
			
			if (curMaterial->m_normalTex != nullptr)
			{
				m_materialConstants[i].NormalTexIndex = static_cast<int>(gTexContainer.GetBindIndex(curMaterial->m_normalTex));
			}

			if (curMaterial->m_roughnessTex != nullptr)
			{
				m_materialConstants[i].RoughnessTexIndex = static_cast<int>(gTexContainer.GetBindIndex(curMaterial->m_roughnessTex));
			}

			if (curMaterial->m_metallicTex != nullptr)
			{
				m_materialConstants[i].MetallicTexIndex = static_cast<int>(gTexContainer.GetBindIndex(curMaterial->m_metallicTex));
			}
			
			if (curMaterial->m_ambientOcclusionTex != nullptr)
			{
				m_materialConstants[i].m_ambientOcclusionTexIndex = static_cast<int>(gTexContainer.GetBindIndex(curMaterial->m_ambientOcclusionTex));
			}
		}
	}

	m_materialConstantsBuffer.UpdateResource(m_materialConstants.data(), static_cast<uint32_t>(m_materialConstants.size()));
}

void RTPipelineResources::UpdateGlobalConstants(GlobalConstants& globalConstnats)
{
	m_globalConstantsBuffer.UpdateResource(globalConstnats);
}