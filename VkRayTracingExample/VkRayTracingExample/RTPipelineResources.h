#pragma once

#include "DeviceBuffers.h"
#include "TextureContainer.h"

struct GlobalConstants
{
	glm::mat4 MatViewInv = glm::mat4(1.0f);
	glm::mat4 MatProjInv = glm::mat4(1.0f);
	glm::vec3 LightDir = glm::vec3(1.0f);
	float Padding0 = 0;
};

class RTPipelineResources
{
public:
	struct InstanceConstants
	{
		glm::mat4 WorldMat;

		int GeometryID;
		int MaterialID;
		float Padding0;
		float Padding1;
	};

	struct MaterialConstants
	{
		glm::vec4 m_color = glm::vec4(1.0f);

		uint32_t MateiralTypeIndex = 0;
		float IndexOfRefraction = 1.0f;
		float Metallic = 0.0f;
		float Roughness = 1.0f;

		int DiffuseTexIndex = -1;
		int NormalTexIndex = -1;
		int RoughnessTexIndex = -1;
		int MetallicTexIndex = -1;

		uint32_t m_ambientOcclusionTexIndex = -1;
		float uvScale = 1.0f;
		float Padding0;
		float Padding1;
	};

public:
	bool Build(VkAccelerationStructureKHR tlasHandle, RtTargetImageBuffer* targetImageBuffer, SimpleCubmapTexture* cubeMap);
	void Update(GlobalConstants& globalConstnats, VkAccelerationStructureKHR tlasHandle = VK_NULL_HANDLE);
	void Destroy();

public:
	bool RefreshResourceBind(VkAccelerationStructureKHR tlasHandle, RtTargetImageBuffer* targetImageBuffer, SimpleCubmapTexture* cubeMap);
	void RefreshWriteDescriptorSet();

protected:
	bool CreateBuffers();
	bool CreateDescriptorPool();
	void DestoryBindLayouts();
	bool RefreshResourceBind();

public:

	VkPipelineLayout GetPipelineLayout() { return m_pipelineLayout; }
	uint32_t GetDescriptorSetCount() { return static_cast<uint32_t>(m_descSets.size()); }
	std::vector<VkDescriptorSet>& GetDescriptorSet() { return m_descSets; }

protected:

	template <typename BufferType>
	void ResizeBuffer(BufferType& buffer, int count);
	void UpdateInstanceConstants();
	void UpdateMaterialConstants();
	void UpdateGlobalConstants(GlobalConstants& globalConstnats);

private:

	VkAccelerationStructureKHR m_tlasHandle = VK_NULL_HANDLE;
	RtTargetImageBuffer* m_targetImageBuffer = nullptr;

	SimpleCubmapTexture* m_skyCubeMap;

	std::vector<InstanceConstants> m_instanceConstants = {};
	StructuredBufferData<InstanceConstants> m_instanceConstantsBuffer = {};

	std::vector<MaterialConstants> m_materialConstants = {};
	StructuredBufferData<MaterialConstants> m_materialConstantsBuffer = {};

	GlobalConstants m_globalConstants = {};
	StructuredBufferData<GlobalConstants> m_globalConstantsBuffer = {};

	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

	VkDescriptorPool m_descPool = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayoutBinding> m_descSetLayoutBindings = {};
	std::vector<VkDescriptorSetLayout> m_descLayouts = {};
	std::vector<VkDescriptorSet> m_descSets = {};

	uint32_t m_numDescriptorSet = 1;
};

template <typename BufferType>
void RTPipelineResources::ResizeBuffer(BufferType& buffer, int count)
{
	if (buffer.IsAllocated())
	{
		buffer.Resize(count);
	}
}