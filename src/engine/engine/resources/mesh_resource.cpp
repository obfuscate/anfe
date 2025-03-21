#include <engine/resources/mesh_resource.h>
#include <engine/helpers.h>
#include <engine/math.h>
#include <engine/services/render_service.h>
#include <engine/services/vfs_service.h>
#include <engine/render/d3d12/backend.h>

#include <ufbx/ufbx.h> //-- ToDo: Add the natvis file.

#define ENABLE_READ_SKINNING 0
#define ENABLE_READ_ANIMATION 0

namespace engine::resources
{

//-- See https://github.com/ufbx/ufbx/blob/master/examples/viewer/viewer.c
namespace
{

#if ENABLE_READ_ANIMATION
struct NodeAnimation
{
	float timeBegin;
	float framerate;
	size_t numFrames;
	math::quat constRot;
	math::vec3 constPos;
	math::vec3 constScale;
	math::quat* rot;
	math::vec3* pos;
	math::vec3* scale;
};

struct BlendChannelAnimation
{
	float constWeight;
	float* weight;
};

struct Animation
{
	const char* name;
	float timeBegin;
	float timeEnd;
	float framerate;
	size_t numFrames;

	NodeAnimation* nodes;
	BlendChannelAnimation* blendChannels;
};
#endif

struct Node
{
	int32_t parent_index;

	math::matrix geometry_to_node;
	math::matrix node_to_parent;
	math::matrix node_to_world;
	math::matrix geometry_to_world;
	math::matrix normal_to_world;
};

struct BlendChannel
{
	float weight;
};

struct Scene
{
	std::vector<Node> nodes;
	std::vector<BlendChannel> blendChannels;
#if ENABLE_READ_ANIMATION
	std::vector<Animation> animations;
#endif

	math::vec3 aabbMin;
	math::vec3 aabbMax;
};


math::vec2 ufbx_to_um_vec2(ufbx_vec2 v) { return math::vec2((float)v.x, (float)v.y); }
math::vec3 ufbx_to_um_vec3(ufbx_vec3 v) { return math::vec3((float)v.x, (float)v.y, (float)v.z); }
//math::vec4 ufbx_to_um_vec4(ufbx_vec4 v) { return math::vec4((float)v.x, (float)v.y, (float)v.z, (float)v.w); }
math::color ufbx_to_um_color(ufbx_vec4 v) { return math::color((float)v.x, (float)v.y, (float)v.z, (float)v.w); }

math::matrix ufbx_to_um_mat(ufbx_matrix m) {
	return math::matrix(
		(float)m.m00, (float)m.m01, (float)m.m02, (float)m.m03,
		(float)m.m10, (float)m.m11, (float)m.m12, (float)m.m13,
		(float)m.m20, (float)m.m21, (float)m.m22, (float)m.m23,
		0.0f, 0.0f, 0.0f, 1.0f
	);
}

void readNode(Node& node, ufbx_node* ufbxNode)
{
	node.parent_index = ufbxNode->parent ? ufbxNode->parent->typed_id : -1;
	node.node_to_parent = ufbx_to_um_mat(ufbxNode->node_to_parent);
	node.node_to_world = ufbx_to_um_mat(ufbxNode->node_to_world);
	node.geometry_to_node = ufbx_to_um_mat(ufbxNode->geometry_to_node);
	node.geometry_to_world = ufbx_to_um_mat(ufbxNode->geometry_to_world);
	node.normal_to_world = ufbx_to_um_mat(ufbx_matrix_for_normals(&ufbxNode->geometry_to_world));
}

void readMesh(MeshResource& mesh, MeshResource::Submesh& submesh, ufbx_mesh_part* meshPart, ufbx_mesh* ufbxMesh, const size_t maxVerticesInStream, const size_t numTrianglesIndices,
	const size_t numUVSets, size_t& vertexOffset, size_t& indexOffset)
{
	ENGINE_ASSERT_DEBUG(ufbxMesh->vertex_position.exists, "FBX mesh doesn't include vertices!");
	std::vector<uint32_t> trianglesIndices(numTrianglesIndices);

	std::array<bool, static_cast<size_t>(MeshResource::Stream::Count)> hasStream =
	{
		true,
		ufbxMesh->vertex_tangent.exists,
		ufbxMesh->vertex_bitangent.exists,
		ufbxMesh->vertex_normal.exists,
		numUVSets > 1,
		numUVSets > 2,
		ufbxMesh->vertex_color.exists
	};

	//-- ToDo: place in a higher scope to avoid redundant reallocations.
	std::vector<math::vec3> positions(maxVerticesInStream);
	//-- ToDo: Pack these values: tangetns, bitangents, normals.
	std::vector<math::vec3> tangents(hasStream[static_cast<size_t>(MeshResource::Stream::Tangent)] ? maxVerticesInStream : 0);
	std::vector<math::vec3> bitangents(hasStream[static_cast<size_t>(MeshResource::Stream::Bitangent)] ? maxVerticesInStream : 0);
	std::vector<math::vec3> normals(hasStream[static_cast<size_t>(MeshResource::Stream::Normal)] ? maxVerticesInStream : 0);
	std::vector<math::vec2> uvSet0(hasStream[static_cast<size_t>(MeshResource::Stream::UV0)] ? maxVerticesInStream : 0);
	std::vector<math::vec2> uvSet1(hasStream[static_cast<size_t>(MeshResource::Stream::UV1)] ? maxVerticesInStream : 0);
	std::vector<uint32_t> colors(hasStream[static_cast<size_t>(MeshResource::Stream::VertexColor)] ? maxVerticesInStream : 0);

	std::vector<uint32_t> indices(maxVerticesInStream);

#if ENABLE_READ_SKINNING
	skin_vertex* skin_vertices = alloc(skin_vertex, max_triangles * 3);
	skin_vertex* mesh_skin_vertices = alloc(skin_vertex, ufbxMesh->num_vertices);
#endif

	// In FBX files a single mesh can be instanced by multiple nodes. ufbx handles the connection
	// in two ways: (1) `ufbx_node.mesh/light/camera/etc` contains pointer to the data "attribute"
	// that node uses and (2) each element that can be connected to a node contains a list of
	// `ufbx_node*` instances eg. `ufbx_mesh.instances`.
	/*mesh.instanceNodeIndices.resize(ufbxMesh->instances.count);
	for (size_t i = 0; i < ufbxMesh->instances.count; i++)
	{
		mesh.instanceNodeIndices[i] = (int32_t)ufbxMesh->instances.data[i]->typed_id;
	}*/

#if ENABLE_READ_SKINNING
	// Create the vertex buffers
	size_t num_blend_shapes = 0;
	ufbx_blend_channel* blend_channels[MAX_BLEND_SHAPES];
	size_t num_bones = 0;
	ufbx_skin_deformer* skin = NULL;*/

	if (ufbxMesh->skin_deformers.count > 0)
	{
		ENGINE_FAIL("Implement skinning!");
		mesh.skinned = true;

		// Having multiple skin deformers attached at once is exceedingly rare so we can just
		// pick the first one without having to worry too much about it.
		skin = ufbxMesh->skin_deformers.data[0];

		// NOTE: A proper implementation would split meshes with too many bones to chunks but
		// for simplicity we're going to just pick the first `MAX_BONES` ones.
		for (size_t ci = 0; ci < skin->clusters.count; ci++) {
			ufbx_skin_cluster* cluster = skin->clusters.data[ci];
			if (num_bones < MAX_BONES) {
				vmesh->bone_indices[num_bones] = (int32_t)cluster->bone_node->typed_id;
				vmesh->bone_matrices[num_bones] = ufbx_to_um_mat(cluster->geometry_to_bone);
				num_bones++;
			}
		}
		vmesh->num_bones = num_bones;

		// Pre-calculate the skinned vertex bones/weights for each vertex as they will probably
		// be shared by multiple indices.
		for (size_t vertexId = 0; vertexId < ufbxMesh->num_vertices; vertexId++) {
			size_t num_weights = 0;
			float total_weight = 0.0f;
			float weights[4] = { 0.0f };
			uint8_t clusters[4] = { 0 };

			// `ufbx_skin_vertex` contains the offset and number of weights that deform the vertex
			// in a descending weight order so we can pick the first N weights to use and get a
			// reasonable approximation of the skinning.
			ufbx_skin_vertex vertex_weights = skin->vertices.data[vertexId];
			for (size_t wi = 0; wi < vertex_weights.num_weights; wi++) {
				if (num_weights >= 4) break;
				ufbx_skin_weight weight = skin->weights.data[vertex_weights.weight_begin + wi];

				// Since we only support a fixed amount of bones up to `MAX_BONES` and we take the
				// first N ones we need to ignore weights with too high `cluster_index`.
				if (weight.cluster_index < MAX_BONES) {
					total_weight += (float)weight.weight;
					clusters[num_weights] = (uint8_t)weight.cluster_index;
					weights[num_weights] = (float)weight.weight;
					num_weights++;
				}
			}

			// Normalize and quantize the weights to 8 bits. We need to be a bit careful to make
			// sure the _quantized_ sum is normalized ie. all 8-bit values sum to 255.
			if (total_weight > 0.0f) {
				skin_vertex* skin_vert = &mesh_skin_vertices[vertexId];
				uint32_t quantized_sum = 0;
				for (size_t i = 0; i < 4; i++) {
					uint8_t quantized_weight = (uint8_t)((float)weights[i] / total_weight * 255.0f);
					quantized_sum += quantized_weight;
					skin_vert->bone_index[i] = clusters[i];
					skin_vert->bone_weight[i] = quantized_weight;
				}
				skin_vert->bone_weight[0] += 255 - quantized_sum;
			}
		}*/
	}

	// Fetch blend channels from all attached blend deformers.
	for (size_t di = 0; di < ufbxMesh->blend_deformers.count; di++)
	{
		ufbx_blend_deformer* deformer = ufbxMesh->blend_deformers.data[di];
		for (size_t ci = 0; ci < deformer->channels.count; ci++)
		{
			ufbx_blend_channel* chan = deformer->channels.data[ci];
			if (chan->keyframes.count == 0)
			{
				continue;
			}

			if (num_blend_shapes < MAX_BLEND_SHAPES)
			{
				blend_channels[num_blend_shapes] = chan;
				vmesh->blend_channel_indices[num_blend_shapes] = (int32_t)chan->typed_id;
				num_blend_shapes++;
			}
		}
	}
	if (num_blend_shapes > 0) {
		vmesh->blend_shape_image = pack_blend_channels_to_image(ufbxMesh, blend_channels, num_blend_shapes);
		vmesh->num_blend_shapes = num_blend_shapes;
	}
#endif

	size_t numVertices = 0;
	//-- First fetch all vertices into a flat non-indexed buffer, we also need to triangulate the faces.
	for (size_t faceId = 0; faceId < meshPart->num_faces; faceId++)
	{
		ufbx_face face = ufbxMesh->faces.data[meshPart->face_indices.data[faceId]];
		//-- Right now we assume that all faces are triangulated.
		uint32_t numTrianglesSubmesh = ufbx_triangulate_face(trianglesIndices.data(), numTrianglesIndices, ufbxMesh, face);
		ENGINE_ASSERT_DEBUG(numTrianglesSubmesh == 1, "Not triangulated face!");

		//-- Iterate through every vertex of every triangle in the triangulated result
		for (uint32_t vertexId = 0; vertexId < numTrianglesSubmesh * 3; vertexId++)
		{
			uint32_t idx = trianglesIndices[vertexId];

			ufbx_vec3 pos = ufbx_get_vertex_vec3(&ufbxMesh->vertex_position, idx);
			positions[numVertices] = ufbx_to_um_vec3(pos);

			if (hasStream[static_cast<size_t>(MeshResource::Stream::Tangent)])
			{
				ufbx_vec3 tangent = ufbx_get_vertex_vec3(&ufbxMesh->vertex_tangent, idx);
				tangents[numVertices] = ufbx_to_um_vec3(tangent).Normalized();
			}
			if (hasStream[static_cast<size_t>(MeshResource::Stream::Bitangent)])
			{
				ufbx_vec3 bitangent = ufbx_get_vertex_vec3(&ufbxMesh->vertex_bitangent, idx);
				bitangents[numVertices] = ufbx_to_um_vec3(bitangent).Normalized();
			}
			if (hasStream[static_cast<size_t>(MeshResource::Stream::Normal)])
			{
				ufbx_vec3 normal = ufbx_get_vertex_vec3(&ufbxMesh->vertex_normal, idx);
				normals[numVertices] = ufbx_to_um_vec3(normal).Normalized();
			}
			if (hasStream[static_cast<size_t>(MeshResource::Stream::UV0)])
			{
				ufbx_vec2 uv0 = ufbx_get_vertex_vec2(&ufbxMesh->uv_sets.data[0].vertex_uv, idx);
				uvSet0[numVertices] = ufbx_to_um_vec2(uv0);
			}
			if (hasStream[static_cast<size_t>(MeshResource::Stream::UV1)])
			{
				ufbx_vec2 uv1 = ufbx_get_vertex_vec2(&ufbxMesh->uv_sets.data[0].vertex_uv, idx);
				uvSet1[numVertices] = ufbx_to_um_vec2(uv1);
			}
			if (hasStream[static_cast<size_t>(MeshResource::Stream::VertexColor)])
			{
				ufbx_vec4 color = ufbx_get_vertex_vec4(&ufbxMesh->vertex_color, idx);
				colors[numVertices] = ufbx_to_um_color(color).BGRA();
			}
			//vert->f_vertex_index = (float)ufbxMesh->vertex_indices.data[idx];

			// The skinning vertex stream is pre-calculated above so we just need to
			// copy the right one by the vertex index.
#if ENABLE_READ_SKINNING
			if (skin) {
				skin_vertices[numVertices] = mesh_skin_vertices[ufbxMesh->vertex_indices.data[idx]];
			}
#endif
			numVertices++;
		}
	}

	std::vector<ufbx_vertex_stream> streams;
	streams.reserve(static_cast<size_t>(MeshResource::Stream::Count));

	auto fillStream = [&streams, numVertices](auto& container)
		{
			auto& stream = streams.emplace_back();

			stream.data = container.data();
			stream.vertex_count = numVertices;
			stream.vertex_size = sizeof(container[0]);
		};

	fillStream(positions);

	if (hasStream[static_cast<size_t>(MeshResource::Stream::Tangent)])
	{
		fillStream(tangents);
	}
	if (hasStream[static_cast<size_t>(MeshResource::Stream::Bitangent)])
	{
		fillStream(bitangents);
	}
	if (hasStream[static_cast<size_t>(MeshResource::Stream::Normal)])
	{
		fillStream(normals);
	}
	if (hasStream[static_cast<size_t>(MeshResource::Stream::UV0)])
	{
		fillStream(uvSet0);
	}
	if (hasStream[static_cast<size_t>(MeshResource::Stream::UV1)])
	{
		fillStream(uvSet1);
	}
	if (hasStream[static_cast<size_t>(MeshResource::Stream::VertexColor)])
	{
		fillStream(colors);
	}

#if ENABLE_READ_SKINNING
	if (skin) {
		streams[1].data = skin_vertices;
		streams[1].vertex_count = numVertices;
		streams[1].vertex_size = sizeof(skin_vertex);
		num_streams = 2;
	}
#endif

	//-- Optimize the flat vertex buffer into an indexed one.
	//-- `ufbx_generate_indices()` compacts the vertex buffer and returns the number of used vertices.
	ufbx_error error;
	size_t numOptimizedVertices = ufbx_generate_indices(streams.data(), streams.size(), indices.data(), numVertices, NULL, &error);
	if (error.type != UFBX_ERROR_NONE)
	{
		logger().error(fmt::format("[MeshResource]: Failed to generate index buffer ({}): {}", static_cast<int32_t>(error.type), error.description.data));
	}

	auto emptyRange = CD3DX12_RANGE(0, 0);
	void* memoryBegin = nullptr;
	size_t streamId = 0;
	for (size_t i = 0; i < static_cast<size_t>(MeshResource::Stream::Count); ++i)
	{
		if (hasStream[i])
		{
			D3D12_VERTEX_BUFFER_VIEW streamView =
			{
				.BufferLocation = mesh.m_streams[i]->GetGPUVirtualAddress(),
				//-- ToDo: looks like we may allocate much more memory than use here.
				//-- See how we calculate totalVertices in the load method.
				.SizeInBytes = static_cast<UINT>(numOptimizedVertices * streams[streamId].vertex_size),
				.StrideInBytes = MeshResource::kStreamSizes[i]
			};
			submesh.renderPart.streamViews[i] = streamView;

			mesh.m_uploadBuffers[i]->Map(0, &emptyRange, &memoryBegin);

			auto* memorySubmesh = static_cast<uint8_t*>(memoryBegin) + vertexOffset * streamView.StrideInBytes;
			memcpy(memorySubmesh, positions.data(), streamView.SizeInBytes);

			mesh.m_uploadBuffers[i]->Unmap(0, nullptr);

			++streamId;
		}
	}

	//-- Initialize the vertex buffer view.
	{
		submesh.renderPart.indexBufferView = {
			.BufferLocation = mesh.m_indexBuffer->GetGPUVirtualAddress(),
			.SizeInBytes = static_cast<UINT>(numVertices * sizeof(uint32_t)),
			//-- TODO: Check the size and put small index buffers to R16 buffer.
			.Format = DXGI_FORMAT_R32_UINT
		};

		mesh.m_uploadIndexBuffer->Map(0, &emptyRange, &memoryBegin);

		auto* memoryIndices = static_cast<uint8_t*>(memoryBegin) + indexOffset * sizeof(uint32_t);
		memcpy(memoryIndices, indices.data(), submesh.renderPart.indexBufferView.SizeInBytes);

		mesh.m_uploadIndexBuffer->Unmap(0, nullptr);
	}

#if ENABLE_READ_SKINNING
	if (vmesh->skinned) {
		part->skin_buffer = sg_make_buffer(&(sg_buffer_desc) {
			.size = num_vertices * sizeof(skin_vertex),
				.type = SG_BUFFERTYPE_VERTEXBUFFER,
				.data = { skin_vertices, num_vertices * sizeof(skin_vertex) },
		});
	}
#endif

	//-- Compute bounds from the vertices.
	//mesh.aabb_is_local = ufbxMesh->skinned_is_local; //-- ToDo: Reconsider later.
	submesh.aabb = math::AABB();
	for (size_t i = 0; i < ufbxMesh->num_vertices; i++)
	{
		math::vec3 pos = ufbx_to_um_vec3(ufbxMesh->skinned_position.values.data[i]);
		submesh.aabb.extend(pos);
	}

	submesh.renderPart.numVertices = static_cast<UINT>(numOptimizedVertices);
	submesh.renderPart.numIndices = static_cast<UINT>(numVertices);
	submesh.renderPart.startIndex = static_cast<UINT>(indexOffset);
	submesh.renderPart.baseVertex = static_cast<UINT>(vertexOffset);

	vertexOffset += numOptimizedVertices;
	indexOffset += numVertices;
}

void readBlendChannel(BlendChannel& blendChannel, ufbx_blend_channel* chan)
{
	blendChannel.weight = (float)chan->weight;
}

#if ENABLE_READ_ANIMATION
void readNodeAnim(Animation& va, NodeAnimation& vna, ufbx_anim_stack* stack, ufbx_node* node)
{
	vna->rot = alloc(um_quat, va->num_frames);
	vna->pos = alloc(um_vec3, va->num_frames);
	vna->scale = alloc(um_vec3, va->num_frames);

	bool const_rot = true, const_pos = true, const_scale = true;

	// Sample the node's transform evenly for the whole animation stack duration
	for (size_t i = 0; i < va->num_frames; i++) {
		double time = stack->time_begin + (double)i / va->framerate;

		ufbx_transform transform = ufbx_evaluate_transform(stack->anim, node, time);
		vna->rot[i] = ufbx_to_um_quat(transform.rotation);
		vna->pos[i] = ufbx_to_um_vec3(transform.translation);
		vna->scale[i] = ufbx_to_um_vec3(transform.scale);

		if (i > 0) {
			// Negated quaternions are equivalent, but interpolating between ones of different
			// polarity takes a the longer path, so flip the quaternion if necessary.
			if (um_quat_dot(vna->rot[i], vna->rot[i - 1]) < 0.0f) {
				vna->rot[i] = um_quat_neg(vna->rot[i]);
			}

			// Keep track of which channels are constant for the whole animation as an optimization
			if (!um_quat_equal(vna->rot[i - 1], vna->rot[i])) const_rot = false;
			if (!um_equal3(vna->pos[i - 1], vna->pos[i])) const_pos = false;
			if (!um_equal3(vna->scale[i - 1], vna->scale[i])) const_scale = false;
		}
	}

	if (const_rot) { vna->const_rot = vna->rot[0]; free(vna->rot); vna->rot = NULL; }
	if (const_pos) { vna->const_pos = vna->pos[0]; free(vna->pos); vna->pos = NULL; }
	if (const_scale) { vna->const_scale = vna->scale[0]; free(vna->scale); vna->scale = NULL; }
}

void readBlendChannelAnim(Animation& va, BlendChannelAnimation* vbca, ufbx_anim_stack* stack, ufbx_blend_channel* chan)
{
	vbca->weight = alloc(float, va->num_frames);

	bool const_weight = true;

	// Sample the blend weight evenly for the whole animation stack duration
	for (size_t i = 0; i < va->num_frames; i++) {
		double time = stack->time_begin + (double)i / va->framerate;

		ufbx_real weight = ufbx_evaluate_blend_weight(stack->anim, chan, time);
		vbca->weight[i] = (float)weight;

		// Keep track of which channels are constant for the whole animation as an optimization
		if (i > 0) {
			if (vbca->weight[i - 1] != vbca->weight[i]) const_weight = false;
		}
	}

	if (const_weight) { vbca->const_weight = vbca->weight[0]; free(vbca->weight); vbca->weight = NULL; }
}

void readAnimStack(Animation& anim, ufbx_anim_stack* stack, ufbx_scene* scene)
{
	const float target_framerate = 30.0f;
	const size_t max_frames = 4096;

	// Sample the animation evenly at `target_framerate` if possible while limiting the maximum
	// number of frames to `max_frames` by potentially dropping FPS.
	float duration = (float)stack->time_end - (float)stack->time_begin;
	size_t num_frames = clamp_sz((size_t)(duration * target_framerate), 2, max_frames);
	float framerate = (float)(num_frames - 1) / duration;

	anim.name = alloc_dup(char, stack->name.length + 1, stack->name.data);
	anim.time_begin = (float)stack->time_begin;
	anim.time_end = (float)stack->time_end;
	anim.framerate = framerate;
	anim.num_frames = num_frames;

	// Sample the animations of all nodes and blend channels in the stack
	anim.nodes = alloc(viewer_node_anim, scene->nodes.count);
	for (size_t i = 0; i < scene->nodes.count; i++) {
		ufbx_node* node = scene->nodes.data[i];
		read_node_anim(anim, anim.nodes[i], stack, node);
	}

	anim.blend_channels = alloc(viewer_blend_channel_anim, scene->blend_channels.count);
	for (size_t i = 0; i < scene->blend_channels.count; i++) {
		ufbx_blend_channel* chan = scene->blend_channels.data[i];
		read_blend_channel_anim(anim, anim.blend_channels[i], stack, chan);
	}
}
#endif
}

void MeshResource::load(std::string_view path)
{
	auto& rs = service<RenderService>();
	std::string absolutePath = service<VFSService>().absolutePath(path);

	ufbx_load_opts opts = {
		.target_axes = ufbx_axes_right_handed_y_up,
		.target_unit_meters = 1.0f,
	};

	ufbx_error error;
	ufbx_scene* ufbxScene = ufbx_load_file(absolutePath.c_str(), &opts, &error);
	if (!ufbxScene)
	{
		logger().error(fmt::format("[MeshResource]: Can't load the file '{}'. Error: {}", absolutePath, error.description.data));
		return ;
	}

	Scene scene;
	scene.nodes.resize(ufbxScene->nodes.count);
	for (size_t i = 0; i < scene.nodes.size(); i++)
	{
		readNode(scene.nodes[i], ufbxScene->nodes.data[i]);
	}

	//-- ToDo: Remove it and use proper API.
	auto* d3d12Backend = static_cast<render::d3d12::Backend*>(rs.backend());
	auto* device = d3d12Backend->device();

	//-- Step1. Prepare: calc some data.
	//-- Assume that all meshes in a file are part of one big mesh.
	size_t totalSubmeshes = 0;
	size_t maxTriangles = 0;
	size_t totalVertices = 0;
	for (size_t meshId = 0; meshId < ufbxScene->meshes.count; meshId++)
	{
		auto* ufbxMesh = ufbxScene->meshes.data[meshId];
		//-- We need to render each material of the mesh in a separate part, so let's count the number of parts and maximum number of triangles needed.
		for (size_t partId = 0; partId < ufbxMesh->material_parts.count; partId++)
		{
			ufbx_mesh_part* part = &ufbxMesh->material_parts.data[partId];
			if (part->num_triangles == 0)
			{
				continue;
			}

			totalSubmeshes += 1;
			maxTriangles = std::max(maxTriangles, part->num_triangles);

			for (size_t faceId = 0; faceId < part->num_faces; faceId++)
			{
				ufbx_face face = ufbxMesh->faces.data[part->face_indices.data[faceId]];
				if (face.num_indices != 3)
				{
					logger().error(fmt::format("[MeshResource]: Mesh {}, submesh {} has got not triangulated face {}.", meshId, partId, faceId));
					return;
				}
			}

			//-- Assume that a model has already triangulated faces.
			totalVertices += part->num_triangles * 3;
		}

		if (maxTriangles == 0)
		{
			logger().error(fmt::format("[MeshResource]: Zero triangles in the mesh {}! Did you forget to triangulate it?", meshId));
			return;
		}
	}

	//-- Step 2. Create GPU resources.
	{
		D3D12_HEAP_PROPERTIES heapProps =
		{
			.Type = D3D12_HEAP_TYPE_UPLOAD,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 1,
			.VisibleNodeMask = 1
		};

		D3D12_RESOURCE_DESC bufferDesc =
		{
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Width = 0,
			.Height = 1,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_UNKNOWN,
			.SampleDesc = {.Count = 1, .Quality = 0},
			.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags = D3D12_RESOURCE_FLAG_NONE
		};

		//-- Stream buffers.
		for (size_t i = 0; i < static_cast<size_t>(MeshResource::Stream::Count); ++i)
		{
			bufferDesc.Width = MeshResource::kStreamSizes[i] * totalVertices;
			m_streamsSize[i] = bufferDesc.Width;

			//-- Upload buffer.
			heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
			assertIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_uploadBuffers[i])));

			//-- Usual stream.
			heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
			assertIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_streams[i])));
		}

		//-- Index buffers.
		{
			//-- TODO: Check the size of index buffer and place it in a dedicated buffer (r16/r32).
			bufferDesc.Width = sizeof(uint32_t) * 10000; //-- ToDo: Reconsider later.
			m_indexBufferSize = bufferDesc.Width;

			//-- Upload buffer.
			heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
			assertIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_uploadIndexBuffer)));

			//-- Usual stream.
			heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
			assertIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_indexBuffer)));
		}
	}

	//-- Step 3. Reading the file.
	{
		m_subMeshes.reserve(totalSubmeshes);
		m_combinedAABB = math::AABB();
		size_t vertexOffset = 0;
		size_t indexOffset = 0;

		const size_t maxVerticesInStream = maxTriangles * 3;
		for (size_t meshId = 0; meshId < ufbxScene->meshes.count; meshId++)
		{
			//-- Our shader supports only a single material per draw call so we need to split the mesh into parts by material.
			//-- `ufbx_mesh_part` contains a handy compact list of faces that use the material which we use here.
			auto* ufbxMesh = ufbxScene->meshes.data[meshId];
			if (ufbxMesh->skin_deformers.count > 0)
			{
				ENGINE_FAIL("Implement skinning!");
				logger().error("Mesh consists of skinning geometry! Loader doesn't support yet this technology (:D)");
			}

			const size_t numTrianglesIndices = ufbxMesh->max_face_triangles * 3;
			const size_t numUVSets = ufbxMesh->uv_sets.count;
			for (size_t partId = 0; partId < ufbxMesh->material_parts.count; partId++)
			{
				ufbx_mesh_part* meshPart = &ufbxMesh->material_parts.data[partId];
				if (meshPart->num_triangles == 0)
				{
					continue;
				}

				auto& submesh = m_subMeshes.emplace_back();

				readMesh(*this, submesh, meshPart, ufbxMesh, maxVerticesInStream, numTrianglesIndices, numUVSets, vertexOffset, indexOffset);
				m_combinedAABB.extend(submesh.aabb);
			}
		}
	}

	//-- Step 4. Upload data to the GPU.
	/*{
		CommandContext& InitContext = CommandContext::Begin();

		size_t MaxBytes = std::min<size_t>(Dest.GetBufferSize() - DestOffset, Src.GetBufferSize() - SrcOffset);
		NumBytes = std::min<size_t>(MaxBytes, NumBytes);

		// copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
		InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST, true);
		InitContext.m_CommandList->CopyBufferRegion(Dest.GetResource(), DestOffset, (ID3D12Resource*)Src.GetResource(), SrcOffset, NumBytes);
		InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ, true);

		// Execute the command list and wait for it to finish so we can release the upload buffer
		InitContext.Finish(true);
	}*/

	scene.blendChannels.resize(ufbxScene->blend_channels.count);
	for (size_t i = 0; i < scene.blendChannels.size(); i++)
	{
		readBlendChannel(scene.blendChannels[i], ufbxScene->blend_channels.data[i]);
	}

#if ENABLE_READ_ANIMATION
	scene.animations.resize(ufbxScene->anim_stacks.count);
	for (size_t i = 0; i < scene.animations.size(); i++)
	{
		readAnimStack(scene.animations[i], ufbxScene->anim_stacks.data[i], ufbxScene);
	}
#endif

	ufbx_free_scene(ufbxScene);

	m_status = Status::Ready;
}

} //-- engine::resources.
