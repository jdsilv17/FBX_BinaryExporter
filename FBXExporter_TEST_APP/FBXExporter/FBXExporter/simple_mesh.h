#pragma once

#include <array>
#include <vector>
#include <cstdint>
#include <DirectXMath.h>

namespace end
{
	//class alignas(8) float2 : public std::array<float, 2> {};

	//class alignas(16) float4 : public std::array<float, 4> {};

	struct simple_vert
	{
		DirectX::XMFLOAT4 pos;
		DirectX::XMFLOAT4 color;
		int joint_index[4];
		float weights[4];
		DirectX::XMFLOAT3 norm;
		DirectX::XMFLOAT2 tex_coord;
	};

	struct simple_mesh
	{
		uint32_t vert_count = 0;
		uint32_t index_count = 0;
		simple_vert* verts = nullptr;
		uint32_t* indices = nullptr;
	};

	struct material_t
	{
		enum e_component { EMISSIVE = 0, DIFFUSE, SPECULAR, SHININESS, COUNT };

		struct component_t
		{
			float value[3] = { 0.0f, 0.0f, 0.0f };
			float factor = 0.0f;
			int64_t input = -1;
		};

		component_t& operator[](int i) { return components[i]; }
		const component_t& operator[](int i)const { return components[i]; }

	private:
		component_t components[COUNT];
	};

	struct myJoint
	{
		FbxNode* node;
		DirectX::XMFLOAT4X4 globalBindposeInverse;
		int parent_index;
	};

	struct Joint
	{
		DirectX::XMFLOAT4X4 global_xform;
		DirectX::XMFLOAT4X4 inverse_xform;
		int parent_index;
	};

	struct myKeyFrame
	{
		double time = 0;

		std::vector<Joint> joints;
	};

	struct AnimClip
	{
		double duration = 0;
		int frameCount = 0;
		std::vector<myKeyFrame> frames;
	};
}
