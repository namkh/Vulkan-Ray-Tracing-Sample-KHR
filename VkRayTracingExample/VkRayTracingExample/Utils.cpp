
#include "Utils.h"
#include "VulkanDeviceResources.h"


bool SimpleFbxGeometiesLoader::Initialize()
{
	m_fbxManager = FbxManager::Create();
	m_fbxIoSettings = FbxIOSettings::Create(m_fbxManager, IOSROOT);
	m_fbxManager->SetIOSettings(m_fbxIoSettings);

	return true;
}

void SimpleFbxGeometiesLoader::Destory()
{
	m_fbxIoSettings->Destroy();
	m_fbxManager->Destroy();
}

void SimpleFbxGeometiesLoader::GetGeometries(fbxsdk::FbxNode* curNode, std::vector<fbxsdk::FbxMesh*>& fbxMeshes)
{
	if (curNode != nullptr)
	{
		fbxsdk::FbxMesh* mesh = curNode->GetMesh();
		if (mesh != nullptr)
		{
			fbxMeshes.push_back(mesh);
		}
		for (int i = 0; i < curNode->GetChildCount(); i++)
		{
			fbxsdk::FbxNode* childNode = curNode->GetChild(i);
			GetGeometries(childNode, fbxMeshes);
		}
	}
}

void SimpleFbxGeometiesLoader::ReadNormal(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, glm::vec3& outNormal)
{
	fbxsdk::FbxGeometryElementNormal* vertexNormal = inMesh->GetElementNormal(0);

	switch (vertexNormal->GetMappingMode())
	{
	case FbxGeometryElement::eByControlPoint:
		switch (vertexNormal->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			outNormal.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[0]);
			outNormal.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[2]);
			outNormal.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[1]);
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexNormal->GetIndexArray().GetAt(inCtrlPointIndex);
			outNormal.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[0]);
			outNormal.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[2]);
			outNormal.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[1]);
		}
		break;
		default:
			throw std::exception("Invalid Reference");
		}
		break;
	case FbxGeometryElement::eByPolygonVertex:
		switch (vertexNormal->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			outNormal.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inVertexCounter).mData[0]);
			outNormal.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inVertexCounter).mData[2]);
			outNormal.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inVertexCounter).mData[1]);
		}
		break;
		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexNormal->GetIndexArray().GetAt(inVertexCounter);
			outNormal.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[0]);
			outNormal.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[2]);
			outNormal.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[1]);
		}
		break;
		default: throw std::exception("Invalid Reference");
		}
		break;
	}
}

void SimpleFbxGeometiesLoader::ReadTangent(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, glm::vec3& outTangent)
{
	fbxsdk::FbxGeometryElementTangent* vertexTangent = inMesh->GetElementTangent(0);

	switch (vertexTangent->GetMappingMode())
	{
	case FbxGeometryElement::eByControlPoint:
		switch (vertexTangent->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			outTangent.x = static_cast<float>(vertexTangent->GetDirectArray().GetAt(inCtrlPointIndex).mData[0]);
			outTangent.y = static_cast<float>(vertexTangent->GetDirectArray().GetAt(inCtrlPointIndex).mData[2]);
			outTangent.z = static_cast<float>(vertexTangent->GetDirectArray().GetAt(inCtrlPointIndex).mData[1]);
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexTangent->GetIndexArray().GetAt(inCtrlPointIndex);
			outTangent.x = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[0]);
			outTangent.y = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[2]);
			outTangent.z = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[1]);
		}
		break;
		}
		break;
	case FbxGeometryElement::eByPolygonVertex:
		switch (vertexTangent->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			outTangent.x = static_cast<float>(vertexTangent->GetDirectArray().GetAt(inVertexCounter).mData[0]);
			outTangent.y = static_cast<float>(vertexTangent->GetDirectArray().GetAt(inVertexCounter).mData[2]);
			outTangent.z = static_cast<float>(vertexTangent->GetDirectArray().GetAt(inVertexCounter).mData[1]);
		}
		break;
		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexTangent->GetIndexArray().GetAt(inVertexCounter);
			outTangent.x = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[0]);
			outTangent.y = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[2]);
			outTangent.z = static_cast<float>(vertexTangent->GetDirectArray().GetAt(index).mData[1]);
		}
		}
		break;
	}
}

void SimpleFbxGeometiesLoader::ReadUV(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, glm::vec2& outUV)
{
	fbxsdk::FbxGeometryElementUV* vertexUV = inMesh->GetElementUV(0);

	switch (vertexUV->GetMappingMode())
	{
	case FbxGeometryElement::eByControlPoint:
		switch (vertexUV->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			outUV.x = static_cast<float>(vertexUV->GetDirectArray().GetAt(inCtrlPointIndex).mData[0]);
			outUV.y = static_cast<float>(vertexUV->GetDirectArray().GetAt(inCtrlPointIndex).mData[1]);
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexUV->GetIndexArray().GetAt(inCtrlPointIndex);
			outUV.x = static_cast<float>(vertexUV->GetDirectArray().GetAt(index).mData[0]);
			outUV.y = static_cast<float>(vertexUV->GetDirectArray().GetAt(index).mData[1]);
		}
		break;
		}
		break;
	case FbxGeometryElement::eByPolygonVertex:
		switch (vertexUV->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			outUV.x = static_cast<float>(vertexUV->GetDirectArray().GetAt(inVertexCounter).mData[0]);
			outUV.y = static_cast<float>(vertexUV->GetDirectArray().GetAt(inVertexCounter).mData[1]);
		}
		break;
		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexUV->GetIndexArray().GetAt(inVertexCounter);
			outUV.x = static_cast<float>(vertexUV->GetDirectArray().GetAt(index).mData[0]);
			outUV.y = static_cast<float>(vertexUV->GetDirectArray().GetAt(index).mData[1]);
		}
		}
		break;
	}
}

void SimpleFbxGeometiesLoader::ReadColor(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, glm::vec4& outColor)
{
	if (inMesh->GetElementVertexColorCount() == 0)
	{
		return;
	}

	fbxsdk::FbxGeometryElementVertexColor* vertexColor = inMesh->GetElementVertexColor(0);
	switch (vertexColor->GetMappingMode())
	{
	case FbxGeometryElement::eByControlPoint:
		switch (vertexColor->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			outColor.x = static_cast<float>(vertexColor->GetDirectArray().GetAt(inCtrlPointIndex).mRed);
			outColor.y = static_cast<float>(vertexColor->GetDirectArray().GetAt(inCtrlPointIndex).mGreen);
			outColor.z = static_cast<float>(vertexColor->GetDirectArray().GetAt(inCtrlPointIndex).mBlue);
			outColor.w = static_cast<float>(vertexColor->GetDirectArray().GetAt(inCtrlPointIndex).mAlpha);
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexColor->GetIndexArray().GetAt(inCtrlPointIndex);
			outColor.x = static_cast<float>(vertexColor->GetDirectArray().GetAt(index).mRed);
			outColor.y = static_cast<float>(vertexColor->GetDirectArray().GetAt(index).mGreen);
			outColor.z = static_cast<float>(vertexColor->GetDirectArray().GetAt(index).mBlue);
			outColor.w = static_cast<float>(vertexColor->GetDirectArray().GetAt(index).mAlpha);
		}
		break;
		}
		break;
	case FbxGeometryElement::eByPolygonVertex:
		switch (vertexColor->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			outColor.x = static_cast<float>(vertexColor->GetDirectArray().GetAt(inVertexCounter).mRed);
			outColor.y = static_cast<float>(vertexColor->GetDirectArray().GetAt(inVertexCounter).mGreen);
			outColor.z = static_cast<float>(vertexColor->GetDirectArray().GetAt(inVertexCounter).mBlue);
			outColor.w = static_cast<float>(vertexColor->GetDirectArray().GetAt(inVertexCounter).mAlpha);
		}
		break;
		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexColor->GetIndexArray().GetAt(inVertexCounter);
			outColor.x = static_cast<float>(vertexColor->GetDirectArray().GetAt(index).mRed);
			outColor.y = static_cast<float>(vertexColor->GetDirectArray().GetAt(index).mGreen);
			outColor.z = static_cast<float>(vertexColor->GetDirectArray().GetAt(index).mBlue);
			outColor.w = static_cast<float>(vertexColor->GetDirectArray().GetAt(index).mAlpha);
		}
		break;
		}
		break;
	}
}

void SimpleFbxGeometiesLoader::Load(std::string filePath, std::vector<FbxGeometryData>& fbxGeometryDatas, bool regenNormalAndTangent)
{
	fbxsdk::FbxScene* fbxScene = FbxScene::Create(m_fbxManager, "LoadScene");

	FbxImporter* fbxImpoter = FbxImporter::Create(m_fbxManager, "LoadImporter");
	fbxImpoter->Initialize(filePath.c_str());
	fbxImpoter->Import(fbxScene);

	fbxsdk::FbxNode* rootNode = fbxScene->GetRootNode();
	if (rootNode != nullptr)
	{
		std::vector<fbxsdk::FbxMesh*> fbxMeshList;
		GetGeometries(rootNode, fbxMeshList);
		if (fbxMeshList.size() > 0)
		{
			fbxGeometryDatas.resize(fbxMeshList.size());
			for (int i = 0; i < fbxMeshList.size(); i++)
			{
				uint32_t controlpointCount = fbxMeshList[i]->GetControlPointsCount();
				int numPolygons = fbxMeshList[i]->GetPolygonCount();

				std::vector<bool> writeTable;
				writeTable.resize(controlpointCount, false);

				fbxGeometryDatas[i].m_positions.resize(controlpointCount);
				fbxGeometryDatas[i].m_normals.resize(controlpointCount);
				fbxGeometryDatas[i].m_tangents.resize(controlpointCount);
				fbxGeometryDatas[i].m_bitangents.resize(controlpointCount);
				fbxGeometryDatas[i].m_color.resize(controlpointCount);
				fbxGeometryDatas[i].m_uv.resize(controlpointCount);

				if (fbxMeshList[i]->GetElementNormalCount() == 0 || regenNormalAndTangent)
				{
					fbxMeshList[i]->GenerateNormals(true);
				}

				if (fbxMeshList[i]->GetElementTangentCount() == 0 || regenNormalAndTangent)
				{
					fbxMeshList[i]->GenerateTangentsDataForAllUVSets(true);
				}

				int vertexCounter = 0;
				//index
				for (int j = 0; j < numPolygons; j++)
				{
					for (int k = 0; k < 3; k++)
					{
						int ctrlPointIdx = fbxMeshList[i]->GetPolygonVertex(j, k);
						fbxGeometryDatas[i].m_indices.push_back(ctrlPointIdx);
						if (!writeTable[ctrlPointIdx])
						{
							fbxGeometryDatas[i].m_positions[ctrlPointIdx] = glm::vec3(fbxMeshList[i]->GetControlPointAt(ctrlPointIdx).mData[0],
								fbxMeshList[i]->GetControlPointAt(ctrlPointIdx).mData[2],
								fbxMeshList[i]->GetControlPointAt(ctrlPointIdx).mData[1]);

							ReadNormal(fbxMeshList[i], ctrlPointIdx, vertexCounter, fbxGeometryDatas[i].m_normals[ctrlPointIdx]);
							ReadTangent(fbxMeshList[i], ctrlPointIdx, vertexCounter, fbxGeometryDatas[i].m_tangents[ctrlPointIdx]);
							ReadColor(fbxMeshList[i], ctrlPointIdx, vertexCounter, fbxGeometryDatas[i].m_color[ctrlPointIdx]);
							ReadUV(fbxMeshList[i], ctrlPointIdx, vertexCounter, fbxGeometryDatas[i].m_uv[ctrlPointIdx]);
							writeTable[ctrlPointIdx] = true;
						}
						++vertexCounter;
					}
				}
			}
		}
	}
	else
	{
		//비어있다는 로깅
	}

	fbxImpoter->Destroy();
	fbxScene->Destroy();
}

void CheckPointManager::SetCheckPoint(VkCommandBuffer cmdBuffer, const char* name, CmdCheckPointType type, int commandBufferIndex)
{
	auto* checkPointData = new CmdCheckPointData(name, type, commandBufferIndex);
	checkPointData->prev = lastCheckPointPtr;
	vkCmdSetCheckpointNV(cmdBuffer, checkPointData);
	lastCheckPointPtr = checkPointData;
}

void CheckPointManager::LoggingCheckPoint(VkQueue queue)
{
	uint32_t numDatas = 0;
	vkGetQueueCheckpointDataNV(queue, &numDatas, nullptr);
	std::vector<VkCheckpointDataNV> datas(numDatas);
	for (auto& cur : datas)
	{
		cur.sType = VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV;
	}
	vkGetQueueCheckpointDataNV(queue, &numDatas, datas.data());
	if (datas.size() != 0)
	{
		for (auto cur : datas)
		{
			OutputDebugStringA("========================================================================\n");
			if (cur.pCheckpointMarker != nullptr)
			{
				CmdCheckPointData* checkPointData = reinterpret_cast<CmdCheckPointData*>(cur.pCheckpointMarker);
				while (checkPointData != nullptr)
				{
					char logBuffer[2048] = {};

					sprintf_s(logBuffer, 2048, "[command buffer index%d]%s\n\0", checkPointData->commandBufferIndex, checkPointData->name);
					OutputDebugStringA(logBuffer);
					checkPointData = checkPointData->prev;
				}
			}
		}
	}
}

UID UniqueKeyGenerator::GetUID()
{
	uint64_t count = ++m_counter;

	auto now = std::chrono::system_clock::now().time_since_epoch();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now);
	uint32_t msi = static_cast<uint32_t>(ms.count());
	uint64_t msi64 = static_cast<uint64_t>(msi);

	return ((count << 32) | (msi64));
}

bool CreateShaderModule(VkDevice device, std::string shaderFileName, VkShaderModule* shaderModule)
{
	std::ifstream shaderFile(shaderFileName, std::ios::binary);
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

			VkResult res = vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, shaderModule);
			if (res == VkResult::VK_SUCCESS)
			{
				return true;
			}
		}
	}
	return false;
}

VkDeviceAddress GetBufferDeviceAddress(VkBuffer buffer)
{
	VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo = {};
	bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR;
	bufferDeviceAddressInfo.buffer = buffer;

	return vkGetBufferDeviceAddressKHR(gLogicalDevice, &bufferDeviceAddressInfo);
}