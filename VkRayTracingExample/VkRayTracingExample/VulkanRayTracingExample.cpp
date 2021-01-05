
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "VulkanRayTracingExample.h"
#include "GlobalSystemValues.h"
#include "TextureContainer.h"
#include "GlobalTimer.h"

bool VulkanRayTracingExample::Initialize()
{
	gFbxGeomLoader.Initialize();
	m_camera.Initialize
	(
		static_cast<float>(GetWidth()),
		static_cast<float>(GetHeight()),
		GlobalSystemValues::Instance().ViewportNearDistance,
		GlobalSystemValues::Instance().ViewportFarDistance,
		GlobalSystemValues::Instance().FovAngleY
	);
	m_camera.SetRadius(50.0f);
	m_camera.SetSeta(PI * 0.5f);
	m_camera.SetPhi(PI * 0.6f);
	m_camera.SetZoomSensitivity(10.0f);
	m_camera.UseInputEvents(true);

	std::vector<std::string> cubemapFilePathList =
	{
		"../Resources/Textures/Sky/left.jpg",
		"../Resources/Textures/Sky/right.jpg",
		"../Resources/Textures/Sky/bottom.jpg",
		"../Resources/Textures/Sky/top.jpg",
		"../Resources/Textures/Sky/back.jpg",
		"../Resources/Textures/Sky/front.jpg",
	};
	if (!m_envCubmapTexture.Initialize(cubemapFilePathList))
	{
		return false;
	}

	m_imageAcquiredSemaphore.resize(NUM_FRAMES);
	m_renderCompleteSemaphore.resize(NUM_FRAMES);
	m_drawFence.resize(NUM_FRAMES);
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		VkSemaphoreCreateInfo semaphoreCrateInfo = {};
		semaphoreCrateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCrateInfo.pNext = nullptr;
		semaphoreCrateInfo.flags = 0;

		bool res = vkCreateSemaphore(gLogicalDevice, &semaphoreCrateInfo, nullptr, &m_imageAcquiredSemaphore[i]) == VkResult::VK_SUCCESS;
		if (!res)
		{
			REPORT(EReportType::REPORT_TYPE_ERROR, "Semaphore create failed");
			return false;
		}
		res = vkCreateSemaphore(gLogicalDevice, &semaphoreCrateInfo, nullptr, &m_renderCompleteSemaphore[i]) == VkResult::VK_SUCCESS;
		if (!res)
		{
			REPORT(EReportType::REPORT_TYPE_ERROR, "Semaphore create failed");
			return false;
		}

		if (!m_drawFence[i].Initialize())
		{
			return false;
		}
	}
	m_imageFence.resize(NUM_FRAMES, VK_NULL_HANDLE);

	glm::mat4 modelRotMat = glm::rotate(glm::mat4(1.0f), PI, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 modelScaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f));

	m_renderObjects.push_back(gRenderObjContainer.CreateRenderObject("../Resources/Mesh/MeetMat.fbx", ExampleMaterialType::EXAMPLE_MAT_TYPE_METAL1));
	m_renderObjects.push_back(gRenderObjContainer.CreateRenderObject("../Resources/Mesh/Plane.fbx", ExampleMaterialType::EXAMPLE_MAT_TYPE_PLATE));
	
	SampleRenderObjectInstance* curInstance = nullptr;
	curInstance = m_renderObjects[0]->CreateInstance(modelRotMat * modelScaleMat);

	curInstance = m_renderObjects[0]->CreateInstance(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 15.0f)) * modelRotMat * modelScaleMat);
	curInstance->SetOverrideMaterial(ExampleMaterialType::EXAMPLE_MAT_TYPE_METAL2);
	m_roteteInstances.push_back(curInstance);
	curInstance = m_renderObjects[0]->CreateInstance(glm::translate(glm::mat4(1.0f), glm::vec3(-15.0f, 0.0f, 0.0f)) * modelRotMat * modelScaleMat);
	curInstance->SetOverrideMaterial(ExampleMaterialType::EXAMPLE_MAT_TYPE_METAL3);
	m_roteteInstances.push_back(curInstance);
	curInstance = m_renderObjects[0]->CreateInstance(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -15.0f)) * modelRotMat * modelScaleMat);
	curInstance->SetOverrideMaterial(ExampleMaterialType::EXAMPLE_MAT_TYPE_GLASS);
	m_roteteInstances.push_back(curInstance);
	curInstance = m_renderObjects[0]->CreateInstance(glm::translate(glm::mat4(1.0f), glm::vec3(15.0f, 0.0f, 0.0f)) * modelRotMat * modelScaleMat);
	curInstance->SetOverrideMaterial(ExampleMaterialType::EXAMPLE_MAT_TYPE_PAINT_TRANSPARENT);
	m_roteteInstances.push_back(curInstance);
	
	curInstance = m_renderObjects[1]->CreateInstance(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * modelRotMat);

	std::string raygenShaderFilePath = "../Resources/Shaders/RayGen.spr";
	std::string defaultMissShaderFilePath = "../Resources/Shaders/Miss.spr";
	std::string shadowMissShaderFilePath = "../Resources/Shaders/ShadowMiss.spr";
	m_rayTracer.Initialize(m_width, m_height, gVkDeviceRes.GetBackbufferFormat());
	m_rayTracer.SetEnvCubemap(&m_envCubmapTexture);
	m_rayTracer.LoadRayGenShader(raygenShaderFilePath);
	m_rayTracer.LoadMissShader(defaultMissShaderFilePath);
	m_rayTracer.LoadMissShader(shadowMissShaderFilePath);
	if (!m_rayTracer.Build())
	{
		return false;
	}

	return true;
}

void VulkanRayTracingExample::Update(float timeDelta)
{
	m_globalConstants.LightDir = glm::vec3(0.0f, 1.0f, -1.0f);
	m_globalConstants.MatViewInv = glm::inverse(m_camera.GetViewMatrix());
	m_globalConstants.MatProjInv = glm::inverse(m_camera.GetProjectionMatrix());
}

void VulkanRayTracingExample::PreRender()
{
	m_rayTracer.Update(m_globalConstants, m_currentFrame);
}

void VulkanRayTracingExample::Render()
{
	if (!m_drawFence[m_currentFrame].WaitForFence())
	{
		return;
	}

	VkResult res = vkAcquireNextImageKHR
	(
		gLogicalDevice,
		VulkanDeviceResources::Instance().GetSwapchain(),
		UINT64_MAX,
		m_imageAcquiredSemaphore[m_currentFrame],
		VK_NULL_HANDLE,
		&m_currentBuffer
	);
	if (res != VkResult::VK_SUCCESS)
	{
		return;
	}

	//같은 펜스 참조인것 같은데...
	if (m_imageFence[m_currentBuffer] != VK_NULL_HANDLE)
	{
		do
		{
			res = vkWaitForFences(gLogicalDevice, 1, &m_imageFence[m_currentBuffer], VK_TRUE, UINT64_MAX);
		} while (res == VK_TIMEOUT);
	}
	m_imageFence[m_currentBuffer] = m_drawFence[m_currentFrame].GetFence();

	if (!m_drawFence[m_currentFrame].Reset())
	{
		//에러 타입 로깅
		return;
	}

	std::vector<VkCommandBuffer> vkCommandBuffers;
	std::vector<CommandBuffer*>& commandBuffers = m_rayTracer.GetWaitCommandBuffer();
	for (auto cur : commandBuffers)
	{
		vkCommandBuffers.push_back(cur->GetCommandBuffer());
		m_drawFence[m_currentFrame].AddSubmittedCommandBuffer(cur);
	}
	
	VkSemaphore signalSemaphore[1] = { m_renderCompleteSemaphore[m_currentFrame] };
	VkSemaphore waitSemaphore[1] = { m_imageAcquiredSemaphore[m_currentFrame] };
	VkPipelineStageFlags pipelineStageFlag[1] = { m_submitPipelineStageFlags };
	VkSubmitInfo submitInfo[1] = {};
	submitInfo[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo[0].pNext = nullptr;
	submitInfo[0].pWaitDstStageMask = pipelineStageFlag;
	submitInfo[0].waitSemaphoreCount = 1;
	submitInfo[0].pWaitSemaphores = waitSemaphore;
	submitInfo[0].commandBufferCount = static_cast<uint32_t>(vkCommandBuffers.size());
	submitInfo[0].pCommandBuffers = vkCommandBuffers.data();
	submitInfo[0].signalSemaphoreCount = 1;
	submitInfo[0].pSignalSemaphores = signalSemaphore;

	res = vkQueueSubmit
	(
		VulkanDeviceResources::Instance().GetGraphicsQueue(),
		1,
		submitInfo,
		m_drawFence[m_currentFrame].GetFence()
	);

	if (res != VkResult::VK_SUCCESS)
	{
		//에러 타입 로깅
		return;
	}

	if (res == VkResult::VK_SUCCESS)
	{
		VkSwapchainKHR swapchains[1] = { VulkanDeviceResources::Instance().GetSwapchain() };
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &m_currentBuffer;

		res = vkQueuePresentKHR(VulkanDeviceResources::Instance().GetPresentQueue(), &presentInfo);
		if (res != VkResult::VK_SUCCESS)
		{
			return;
		}
	}
	m_currentFrame = (m_currentFrame + 1) % NUM_FRAMES;
}

void VulkanRayTracingExample::Destroy()
{
	gVkDeviceRes.WaitForAllDeviceAction();
	m_camera.Destroy();
	m_envCubmapTexture.Destroy();
	for (int i = 0; i < NUM_FRAMES; i++)
	{
		vkDestroySemaphore(gLogicalDevice, m_imageAcquiredSemaphore[i], NULL);
		vkDestroySemaphore(gLogicalDevice, m_renderCompleteSemaphore[i], NULL);
		m_drawFence[i].Destory();
	}

	m_rayTracer.Destroy();

	gRenderObjContainer.Clear();
	gMaterialContainer.Clear();
	gHitGroupContainer.Clear();
	gShaderContainer.Clear();
	gGeomContainer.Clear();
	gTexContainer.Clear();
}

void VulkanRayTracingExample::OnScreenSizeChanged()
{
	m_currentBuffer = 0;
	m_currentFrame = 0;
}