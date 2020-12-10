#pragma once

#include <functional>

#include "DeviceBuffers.h"
#include "Singleton.h"

class SimpleMeshData : public UniqueIdentifier
{
	friend class GeometryContainer;
public:
	AsVertexBuffer* GetVertexBuffer() { return &m_vertexBuffer; }
	AsIndexBuffer* GetIndexBuffer() { return &m_indexBuffer; }

protected:
	bool Load(FbxGeometryData& geometryData);
	void Unload();

	void OnUpdated();

	LambdaCommandWithOneParam<std::function<void(uint32_t)>, uint32_t> OnMeshUpdated;

private:
	AsVertexBuffer m_vertexBuffer;
	AsIndexBuffer m_indexBuffer;
};

class SimpleGeometry : public RefCounter, public UniqueIdentifier
{
	friend class GeometryContainer;
public:
	void Destroy();

	void SetSrcFilePath(std::string& srcFilePath) { m_srcFilePath = srcFilePath; }
	std::string GetSrcFilePath() { return m_srcFilePath; }

	uint32_t GetMeshCount() { return static_cast<uint32_t>(m_meshList.size()); }
	SimpleMeshData* GetMesh(uint32_t index);

protected:
	bool Load(std::string& fbxFilePath);
	void Unload();

private:

	std::vector<SimpleMeshData*> m_meshList = {};
	std::string m_srcFilePath = "";
};
