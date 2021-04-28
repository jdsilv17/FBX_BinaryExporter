#pragma once
#include "fbxsdk.h"
#include "simple_mesh.h"

namespace FBXUtils
{
	FbxManager* sdk_manager = nullptr;

	FbxScene* Scene = nullptr;

	FbxManager* Create_and_Import(const char* fbx_file_path, FbxScene*& lScene);

	void ReadNormals(fbxsdk::FbxMesh* pMesh, int inVertexCount, int inPointIndex, DirectX::XMFLOAT3& out_norm);
	
	void ReadUVs(fbxsdk::FbxMesh* pMesh, int inVertexCount, int inPointIndex, DirectX::XMFLOAT2& out_uv);

	int Process_Mesh(FbxNode* Node, const char* output_file_path);

	int Process_Animation(const char* output_file_path);

	void Compactify(end::simple_mesh& simpleMesh);

	void Export_Mesh_File(end::simple_mesh mesh, const char* output_file_path);

	void Export_Animation_File(end::AnimClip* animClip, const char* output_file_path);
}