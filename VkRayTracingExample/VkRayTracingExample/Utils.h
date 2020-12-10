#pragma once

#include <fstream>
#include <vector>
#include <math.h>
#include <chrono>
#include <map>

#include "ExternalLib.h"
#include "Singleton.h"

#define INVALID_INDEX_INT INT_MIN
#define INVALID_COMMAND_HANDLE UINT32_MAX

#define RESOURCE_CONTAINER_INITIAL_SIZE 256

#define PI 3.14159265359f

//ifdef 필요한가?
#define IDENTITY_FLOAT4X4			\
glm::mat4							\
(									\
	1.0f, 0.0f, 0.0f, 0.0f,			\
	0.0f, 1.0f, 0.0f, 0.0f,			\
	0.0f, 0.0f, 1.0f, 0.0f,			\
	0.0f, 0.0f, 0.0f, 1.0f			\
)									\

#define NUM_SAMPLES VK_SAMPLE_COUNT_1_BIT
#define FENCE_TIMEOUT 1000000

class RefCounter
{
public:
	void IncRef() { ++m_refCount; }
	void DecRef()
	{
		if (m_refCount > 0)
		{
			--m_refCount;
		}
	}

	uint32_t GetRefCount() { return m_refCount; }

public:
	uint32_t m_refCount = 0;
};

struct FbxGeometryData
{
	std::vector<glm::vec3> m_positions;
	std::vector<uint32_t> m_indices;
	std::vector<glm::vec3> m_normals;
	std::vector<glm::vec3> m_tangents;
	std::vector<glm::vec3> m_bitangents;
	std::vector<glm::vec4> m_color;
	std::vector<glm::vec2> m_uv;
};

class SimpleFbxGeometiesLoader : public TSingleton<SimpleFbxGeometiesLoader>
{
public:
	SimpleFbxGeometiesLoader(token) {};

public:
	bool Initialize();
	void Destory();

protected:
	void GetGeometries(fbxsdk::FbxNode* curNode, std::vector<fbxsdk::FbxMesh*>& fbxMeshes);

	void ReadNormal(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, glm::vec3& outNormal);
	void ReadTangent(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, glm::vec3& outTangent);
	void ReadUV(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, glm::vec2& outUV);
	void ReadColor(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, glm::vec4& outColor);

public:
	void Load(std::string filePath, std::vector<FbxGeometryData>& fbxGeometryDatas, bool regenNormalAndTangent = false);

private:

	fbxsdk::FbxManager* m_fbxManager = nullptr;
	fbxsdk::FbxIOSettings* m_fbxIoSettings = nullptr;
};

#define gFbxGeomLoader SimpleFbxGeometiesLoader::Instance()

enum class CmdCheckPointType : uint8_t
{
	BEGIN_RENDER_PASS,
	END_RENDER_PASS,
	DRAW,
	COMPUTE,
	GENERIC
};

struct CmdCheckPointData
{
	CmdCheckPointData(const char* name, CmdCheckPointType type, uint32_t cmdIndex) :
		type(type),
		prev(nullptr)
	{
		sprintf_s(this->name, 2048, "%s\n", name);
		commandBufferIndex = cmdIndex;
	}

	char name[2048];
	CmdCheckPointType type;
	uint32_t commandBufferIndex = 0;
	CmdCheckPointData* prev;
};

class CheckPointManager
{
public:
	void SetCheckPoint(VkCommandBuffer cmdBuffer, const char* name, CmdCheckPointType type, int commandBufferIndex);
	void LoggingCheckPoint(VkQueue queue);
	
	CmdCheckPointData* lastCheckPointPtr = nullptr;
	std::vector<CmdCheckPointData> m_checkPointDatas;
};

typedef uint64_t UID;
class UniqueKeyGenerator : public TSingleton<UniqueKeyGenerator>
{
public:
	UniqueKeyGenerator(token) {};
	UID GetUID();

public:
	uint32_t m_counter = 0;
};

class UniqueIdentifier
{
public:
	UniqueIdentifier()
	{
		m_udi = UniqueKeyGenerator::Instance().GetUID();
	}
	UID GetUID()
	{
		return m_udi;
	}
private:

	UID m_udi = 0;
};

struct BufferResouceRange
{
	VkDeviceSize Offset = 0;
	VkDeviceSize Size = 0;
};

bool CreateShaderModule(VkDevice device, std::string shaderFileName, VkShaderModule* shaderModule);
VkDeviceAddress GetBufferDeviceAddress(VkBuffer buffer);
