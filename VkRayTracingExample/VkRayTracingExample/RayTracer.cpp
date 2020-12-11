#include "RayTracer.h"
#include "VulkanDeviceResources.h"
#include "TextureContainer.h"
#include "PipelineBarrier.h"

bool RayTracer::Initialize(uint32_t width, uint32_t height, VkFormat rtTargetFormat)
{
	m_width = width;
	m_height = height;

	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = gVkDeviceRes.GetGraphicsQueueFamilyIndex();
	if (vkCreateCommandPool(gLogicalDevice, &commandPoolCreateInfo, nullptr, &m_commandPool) != VkResult::VK_SUCCESS)
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Command pool create failed.");
		return false;
	}

	if (!m_rtTargetImage.Initialize(m_width, m_height, rtTargetFormat))
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Ray tracing render target image create failed.");
		return false;
	}
	
	if (!m_commandBufferContainer.Initialize(m_commandPool, 2))
	{
		return false;
	}
	
	m_screenSizeChangedEventHandle = gVkDeviceRes.OnRenderTargetSizeChanged.Add
	(
		[this]()
		{
			OnScreenSizeChanged(gVkDeviceRes.GetWidth(), gVkDeviceRes.GetHeight());
		}
	);

	return true;
}

void RayTracer::Update(GlobalConstants& globalConstants, uint32_t frameIndex)
{
	m_accelerationStructure.Update();

	m_currentCommandBuffers.clear();
	if (m_accelerationStructure.HasWaitingCommandToBuild())
	{
		CommandBuffer* buildCmdBuffer = m_accelerationStructure.GetBuildCommandBuffer();
		if (buildCmdBuffer != nullptr)
		{
			m_currentCommandBuffers.push_back(buildCmdBuffer);
		}
		m_accelerationStructure.NotifyBuildCommandSubmitted();
	}
	CommandBuffer* renderCmdBuffer = m_commandBufferContainer.GetCommandBuffer(frameIndex);
	m_currentCommandBuffers.push_back(renderCmdBuffer);

	if (m_accelerationStructure.IsPipelineResourceUpdated())
	{
		m_pipelineResources.Update(globalConstants, m_accelerationStructure.GetTopLevelAs().GetAccelerationStructure());
		m_pipeline.Build(m_pipelineResources.GetPipelineLayout());
		m_shaderBindingTable.Refresh();

		RebuildCommandBuffer();
	}
	else
	{
		m_pipelineResources.Update(globalConstants, VK_NULL_HANDLE);
	}
}


void RayTracer::Destroy()
{
	gVkDeviceRes.OnRenderTargetSizeChanged.Remove(m_screenSizeChangedEventHandle);
	
	for (auto& cur : m_rayGenShaders)
	{
		cur->Destroy();
	}
	for (auto& cur : m_missShaders)
	{
		cur->Destroy();
	}

	m_shaderBindingTable.Destory();
	m_pipeline.Destroy();
	m_pipelineResources.Destroy();
	m_accelerationStructure.Destroy();
	m_commandBufferContainer.Clear();

	m_rtTargetImage.Destroy();
	if (m_commandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(gLogicalDevice, m_commandPool, nullptr);
	}
}

void RayTracer::LoadRayGenShader(std::string& rgenFilePath)
{
	m_rayGenShaders.push_back(gShaderContainer.CreateShader(SHADER_TYPE_RAY_GEN, rgenFilePath));
}

void RayTracer::LoadMissShader(std::string& missFilePath)
{
	m_missShaders.push_back(gShaderContainer.CreateShader(SHADER_TYPE_MISS, missFilePath));
}

bool RayTracer::Build()
{
	if (!m_accelerationStructure.Initialize(m_commandPool))
	{
		return false;
	}

	if (!m_accelerationStructure.Build())
	{
		return false;
	}

	if (!m_pipelineResources.Build(m_accelerationStructure.GetTopLevelAs().GetAccelerationStructure(),
								   &m_rtTargetImage,
								   m_envCubmapTexture))
	{
		return false;
	}

	if (!m_pipeline.Build(m_pipelineResources.GetPipelineLayout()))
	{
		return false;
	}

	if (!m_shaderBindingTable.Build(&m_pipeline))
	{
		return false;
	}

	if (!BuildCommandBuffers())
	{
		REPORT(EReportType::REPORT_TYPE_ERROR, "Command buffer create failed.");
		return false;
	}

	return true;
}

void RayTracer::RebuildCommandBuffer()
{
	m_commandBufferContainer.Reset();
	BuildCommandBuffers();
}

bool RayTracer::BuildCommandBuffers()
{
	VkImageSubresourceRange subResourceRange = {};
	subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResourceRange.baseMipLevel = 0;
	subResourceRange.levelCount = 1;
	subResourceRange.baseArrayLayer = 0;
	subResourceRange.layerCount = 1;

	uint32_t commandBufferCount = m_commandBufferContainer.GetCommandBufferCount();
	for (uint32_t i = 0; i < commandBufferCount; i++)
	{
		CommandBuffer* curCmdBuffer = m_commandBufferContainer.GetCommandBuffer(i);
		if (curCmdBuffer == nullptr)
		{
			return false;
		}

		if (curCmdBuffer->Begin())
		{
			VkImage curBackBuffer = VulkanDeviceResources::Instance().GetSwapChainBuffer(i)->m_image;
			VkCommandBuffer vkCmdBuf = curCmdBuffer->GetCommandBuffer();

			PipelineBarrier pipeLineBarrier(curCmdBuffer->GetCommandBuffer());
			pipeLineBarrier.SetAccessMask(0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, curBackBuffer);
			pipeLineBarrier.SetLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, curBackBuffer);
			pipeLineBarrier.SetImageSubresouceRange(subResourceRange, curBackBuffer);
			pipeLineBarrier.Write();

			vkCmdBindPipeline(vkCmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_pipeline.GetPipeline());

			vkCmdBindDescriptorSets
			(
				vkCmdBuf,
				VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
				m_pipelineResources.GetPipelineLayout(),
				0,
				m_pipelineResources.GetDescriptorSetCount(),
				m_pipelineResources.GetDescriptorSet().data(),
				0,
				nullptr
			);

			VkStridedBufferRegionKHR callableShaderSbtEntry = {};
			vkCmdTraceRaysKHR
			(
				vkCmdBuf,
				m_shaderBindingTable.GetStridedBufferRegion(SHADER_GROUP_TYPE_RAY_GEN),
				m_shaderBindingTable.GetStridedBufferRegion(SHADER_GROUP_TYPE_MISS),
				m_shaderBindingTable.GetStridedBufferRegion(SHADER_GROUP_TYPE_HIT),
				m_shaderBindingTable.GetStridedBufferRegion(SHADER_GROUP_TYPE_CALLABLE),
				m_width,
				m_height,
				1
			);

			pipeLineBarrier.SetAccessMask(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, curBackBuffer);
			pipeLineBarrier.SetLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, curBackBuffer);
			pipeLineBarrier.SetImageSubresouceRange(subResourceRange, curBackBuffer);

			pipeLineBarrier.SetAccessMask(0, VK_ACCESS_TRANSFER_READ_BIT, m_rtTargetImage.GetImage());
			pipeLineBarrier.SetLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_rtTargetImage.GetImage());
			pipeLineBarrier.SetImageSubresouceRange(subResourceRange, m_rtTargetImage.GetImage());

			pipeLineBarrier.Write();

			VkImageCopy copyRegion = {};
			copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.srcSubresource.mipLevel = 0;
			copyRegion.srcSubresource.baseArrayLayer = 0;
			copyRegion.srcSubresource.layerCount = 1;
			copyRegion.srcOffset = { 0, 0, 0 };
			copyRegion.dstSubresource = copyRegion.srcSubresource;
			copyRegion.dstOffset = { 0, 0, 0 };
			copyRegion.extent = { static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height), 1 };
			vkCmdCopyImage
			(
				vkCmdBuf,
				m_rtTargetImage.GetImage(),
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				curBackBuffer,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&copyRegion
			);

			pipeLineBarrier.SetAccessMask(VK_ACCESS_TRANSFER_WRITE_BIT, 0, curBackBuffer);
			pipeLineBarrier.SetLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, curBackBuffer);
			pipeLineBarrier.SetImageSubresouceRange(subResourceRange, curBackBuffer);

			pipeLineBarrier.SetAccessMask(VK_ACCESS_TRANSFER_READ_BIT, 0, m_rtTargetImage.GetImage());
			pipeLineBarrier.SetLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, m_rtTargetImage.GetImage());
			pipeLineBarrier.SetImageSubresouceRange(subResourceRange, m_rtTargetImage.GetImage());

			pipeLineBarrier.Write();

			if (!curCmdBuffer->End())
			{
				return false;
			}
		}
	}
	return true;
}

void RayTracer::OnScreenSizeChanged(uint32_t width, uint32_t height)
{
	m_width		= width;
	m_height	= height;

	m_rtTargetImage.Resize(m_width, m_height);

	m_pipelineResources.RefreshWriteDescriptorSet();
	RebuildCommandBuffer();
}