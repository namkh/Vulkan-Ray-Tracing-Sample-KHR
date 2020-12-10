#include "SimpleGeometry.h"
#include "GeometryContainer.h"

void SimpleGeometry::Destroy()
{
	DecRef();
}

SimpleMeshData* SimpleGeometry::GetMesh(uint32_t index)
{
	if (index < m_meshList.size())
	{
		return m_meshList[index];
	}
	return nullptr;
}

bool SimpleGeometry::Load(std::string& fbxFilePath)
{
	std::vector<FbxGeometryData> geomDatas;
	gFbxGeomLoader.Load(fbxFilePath, geomDatas);

	for (auto& cur : geomDatas)
	{
		SimpleMeshData* meshData = gGeomContainer.LoadMesh(cur);
		m_meshList.push_back(meshData);
	}
	m_srcFilePath = fbxFilePath;
	SetSrcFilePath(fbxFilePath);

	return true;
}

void SimpleGeometry::Unload()
{
	for (auto& cur : m_meshList)
	{
		gGeomContainer.UnloadMesh(cur);
	}
	m_meshList.clear();
}

bool SimpleMeshData::Load(FbxGeometryData& geometryData)
{
	std::vector<DefaultVertex> verts(geometryData.m_positions.size());
	std::vector<uint32_t> indices(geometryData.m_indices);
	
	for (int i = 0; i < geometryData.m_positions.size(); i++)
	{
		verts[i].m_position = glm::vec4(geometryData.m_positions[i], 1.0f);
		if (geometryData.m_normals.size() != 0)
		{
			verts[i].m_normal = glm::vec4(geometryData.m_normals[i], 0.0f);
		}
		if (geometryData.m_tangents.size() != 0)
		{
			verts[i].m_tangent = glm::vec4(geometryData.m_tangents[i], 0.0f);
		}
		if (geometryData.m_color.size() != 0)
		{
			verts[i].m_color = geometryData.m_color[i];
		}
		if (geometryData.m_uv.size() != 0)
		{
			verts[i].m_texcoord = glm::vec4(geometryData.m_uv[i], 0.0f, 0.0f);
		}
	}

	if (!m_vertexBuffer.Initialzie(verts))
	{
		return false;
	}

	if (!m_indexBuffer.Initialzie(indices))
	{
		return false;
	}

	return true;
}

void SimpleMeshData::Unload()
{
	gVkDeviceRes.GraphicsQueueWaitIdle();

	m_vertexBuffer.Destroy();
	m_indexBuffer.Destroy();
}

void SimpleMeshData::OnUpdated()
{
	if (OnMeshUpdated.IsBinded())
	{
		int index = gGeomContainer.GetMeshBindIndex(this);
		if (index != -1)
		{
			OnMeshUpdated.Exec(static_cast<uint32_t>(index));
		}
	}
}