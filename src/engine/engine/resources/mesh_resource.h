#pragma once

#include <engine/resources/resource.h>
#include <engine/math/aabb.h>

//-- TODO: RECONSIDER LATER.
#include <engine/integration/d3d12/integration.h>

namespace engine::resources
{

class MeshResource : public IResource
{
public:
	enum class Stream : uint8_t
	{
		Position,
		Tangent,
		Bitangent,
		Normal,
		UV0,
		UV1,
		VertexColor,
		Count
	};

	inline static constexpr std::array<UINT, static_cast<size_t>(Stream::Count)> kStreamSizes = {
		sizeof(math::vec3), //-- position
		sizeof(math::vec3), //-- tangent
		sizeof(math::vec3), //-- bitangent
		sizeof(math::vec3), //-- normal
		sizeof(math::vec2), //-- uv0
		sizeof(math::vec2), //-- uv1
		sizeof(uint32_t), //-- color
	};

	struct RenderRepresentation
	{
		std::array<D3D12_VERTEX_BUFFER_VIEW, static_cast<size_t>(Stream::Count)> streamViews;
		D3D12_INDEX_BUFFER_VIEW indexBufferView;

		uint32_t numVertices = 0;
		uint32_t numIndices = 0;
		uint32_t startIndex = 0;
		uint32_t baseVertex = 0;
	};

	struct Submesh
	{
		RenderRepresentation renderPart;
		math::AABB aabb;
	};
	using Submeshes = std::vector<Submesh>;

public:
	~MeshResource() = default;

	void load(std::string_view path);

public:
	using Buffer = Microsoft::WRL::ComPtr<ID3D12Resource>;
	//-- Store all streams of each type in a separated combined buffer.
	std::array<Buffer, static_cast<size_t>(Stream::Count)> m_streams;
	std::array<Buffer, static_cast<size_t>(Stream::Count)> m_uploadBuffers; //-- ToDo: Reconsider later. It should be part of Backend/ResourceManager/something else.
	std::array<UINT64, static_cast<size_t>(Stream::Count)> m_streamsSize; //-- ToDo: Reconsider later. It should be part of Backend/ResourceManager/something else.
	//-- Store all indices of all submeshes in one combined buffer.
	Buffer m_indexBuffer;
	Buffer m_uploadIndexBuffer; //-- ToDo: Get rid off. See m_uploadBuffers.
	UINT64 m_indexBufferSize;

	std::vector<Submesh> m_subMeshes;
	math::AABB m_combinedAABB; //-- ToDo: Or jsut calc it every time?
};

using MeshResourcePtr = std::shared_ptr<MeshResource>;

} //-- engine::resources.
