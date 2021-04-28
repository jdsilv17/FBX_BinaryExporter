#include "pch.h"
#include "./Interface/FBX_Export_Interface.h"
#include "FBX_Utilities.h"

#include <vector>
#include <fstream>
#include <list>


namespace FBXUtils
{
	FbxManager* Create_and_Import(const char* fbx_file_path, FbxScene*& lScene)
	{
		// Initialize the SDK manager. This object handles all our memory management.
		FbxManager* lSdkManager = FbxManager::Create();
		// Create the IO settings object.
		FbxIOSettings* IOs = FbxIOSettings::Create(lSdkManager, IOSROOT);
		lSdkManager->SetIOSettings(IOs);
		// Create an importer using the SDK manager.
		FbxImporter* lImporter = FbxImporter::Create(lSdkManager, "");
		// Use the first argument as the filename for the importer.
		if (!lImporter->Initialize(fbx_file_path, -1, lSdkManager->GetIOSettings())) 
		{
			//printf("Call to FbxImporter::Initialize() failed.\n");
			//printf("Error returned: %s\n\n", lImporter->GetStatus().GetErrorString());
			return nullptr;
		}
		// Create a new scene so that it can be populated by the imported file.
		lScene = FbxScene::Create(lSdkManager, "imported_scene");
		// Import the contents of the file into the scene.
		lImporter->Import(lScene);
		lImporter->Destroy();
		return lSdkManager;
	}

	void ReadNormals(fbxsdk::FbxMesh* pMesh, int inVertexCount, int inPointIndex, DirectX::XMFLOAT3& out_norm)
	{
		// get pMesh normals
		// get normal element
		auto* normal_element = pMesh->GetElementNormal();

		if (normal_element)
		{
			// fbx vector to read in normals
			FbxVector4 normal;

			switch (normal_element->GetMappingMode())
			{
			// when we dont have sharp edges so each control point only has one normal
			case FbxGeometryElement::eByControlPoint:
			{
				if (normal_element->GetReferenceMode() == FbxGeometryElement::eDirect)
				{
					normal = normal_element->GetDirectArray().GetAt(inPointIndex);
				}
				else if (normal_element->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
				{
					int norm_index = normal_element->GetIndexArray().GetAt(inPointIndex);
					normal = normal_element->GetDirectArray().GetAt(norm_index);
				}
				else
				{
					throw std::exception("Invalid Reference");
					break;
				}
				out_norm.x = static_cast<float>(normal.mData[0]);
				out_norm.y = static_cast<float>(normal.mData[1]);
				out_norm.z = static_cast<float>(normal.mData[2]);
			}
			break;
			// when we have sharp edges with one control point having multiple normals 
			// and need to get normals of each vertex on each face
			case FbxGeometryElement::eByPolygonVertex:
			{
				if (normal_element->GetReferenceMode() == FbxGeometryElement::eDirect)
				{
					normal = normal_element->GetDirectArray().GetAt(inVertexCount);
				}
				else if (normal_element->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
				{
					int norm_index = normal_element->GetIndexArray().GetAt(inVertexCount);
					normal = normal_element->GetDirectArray().GetAt(norm_index);
				}
				else
				{
					throw std::exception("Invalid Reference");
					break;
				}
				out_norm.x = static_cast<float>(normal.mData[0]);
				out_norm.y = static_cast<float>(normal.mData[1]);
				out_norm.z = static_cast<float>(normal.mData[2]);
			}
			break;
			default:
				break;
			}
		}
	}

	void ReadUVs(fbxsdk::FbxMesh* pMesh, int inVertexCount, int inPointIndex, DirectX::XMFLOAT2& out_uv)
	{
		// get pMesh UVs
		// get uv element
		auto* UV_element = pMesh->GetElementUV();

		if (UV_element)
		{
			// fbx vector to read in UVs
			FbxVector2 tex_coord;

			switch (UV_element->GetMappingMode())
			{
				// when we dont have sharp edges so each control point only has one normal
			case FbxGeometryElement::eByControlPoint:
			{
				if (UV_element->GetReferenceMode() == FbxGeometryElement::eDirect)
				{
					tex_coord = UV_element->GetDirectArray().GetAt(inPointIndex);
				}
				else if (UV_element->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
				{
					int norm_index = UV_element->GetIndexArray().GetAt(inPointIndex);
					tex_coord = UV_element->GetDirectArray().GetAt(norm_index);
				}
				else
				{
					throw std::exception("Invalid Reference");
					break;
				}
				out_uv.x = static_cast<float>(tex_coord.mData[0]);
				out_uv.y = static_cast<float>(tex_coord.mData[1]);
			}
			break;
			// when we have sharp edges with one control point having multiple normals 
			// and need to get normals of each vertex on each face
			case FbxGeometryElement::eByPolygonVertex:
			{
				if (UV_element->GetReferenceMode() == FbxGeometryElement::eDirect)
				{
					tex_coord = UV_element->GetDirectArray().GetAt(inVertexCount);
				}
				else if (UV_element->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
				{
					int norm_index = UV_element->GetIndexArray().GetAt(inVertexCount);
					tex_coord = UV_element->GetDirectArray().GetAt(norm_index);
				}
				else
				{
					throw std::exception("Invalid Reference");
					break;
				}
				out_uv.x = static_cast<float>(tex_coord.mData[0]);
				out_uv.y = static_cast<float>(tex_coord.mData[1]);
			}
			break;
			default:
				break;
			}
		}
	}

	//	2.Extract vertex data from the mesh and store the data in a 'simple_mesh' object or similar
	//		a. SEE "EXPORTING GUIDE.PDF" for pseudocode and in-depth explaination
	//		a. See SDK example code VBOMesh::Initialize() in <FbxSDK Directory>\Samples\ViewScene\SceneCache.cxx
	int Process_Mesh(FbxNode* Node, const char* output_file_path)
	{
		int result = -1;
		int chidlrenCount = Node->GetChildCount();

		int pose_count = FBXUtils::Scene->GetPoseCount();
		FbxPose* pose = nullptr;
		for (int i = 0; i < pose_count; ++i)
		{
			pose = FBXUtils::Scene->GetPose(i);
			if (pose->IsBindPose())
				break;
		}
		if (!pose || !pose->IsBindPose())
			return -1;

		for (int i = 0; i < chidlrenCount; i++)
		{
			FbxNode* childNode = Node->GetChild(i);
			FbxMesh* pMesh = childNode->GetMesh();

			if (pMesh)
			{
				std::vector<end::simple_vert> output;
				
				// number of polygons in pMesh
				int poly_count = pMesh->GetPolygonCount();
				// number of vertices per polygon
				int numIndices = pMesh->GetPolygonVertexCount();
				// index list for vertices
				int* vert_indices = pMesh->GetPolygonVertices();
				// the vertex positions
				FbxVector4 const* control_points = pMesh->GetControlPoints();

				#pragma region ANIMATION SKINNING
				// Animation Skinning===========================================
				const int MAX_INFLUENCES = 4; // max number of joints influencing a single vertex
				struct influence { int joint; float weight; };
				using influence_set = std::array<influence, MAX_INFLUENCES>;
				std::vector<influence_set> control_point_influences;

				//// find bind pose
				//int pose_count = FBXUtils::Scene->GetPoseCount();
				//FbxPose* pose = nullptr;
				//for (int i = 0; i < pose_count; ++i)
				//{
				//	pose = FBXUtils::Scene->GetPose(i);
				//	if (pose->IsBindPose())
				//		break;
				//}
				//if (!pose || !pose->IsBindPose())
				//	return -1;

				// find Skeleton
				int numItems = pose->GetCount();
				FbxSkeleton* skeleton = nullptr;
				for (int i = 0; i < numItems; ++i)
				{
					skeleton = pose->GetNode(i)->GetSkeleton();
					if (skeleton && skeleton->IsSkeletonRoot())
						break;
				}
				if (!skeleton || !skeleton->IsSkeletonRoot())
					return -1;

				std::vector<end::myJoint> JointNodes;

				end::myJoint root;
				root.node = skeleton->GetNode(0);
				root.parent_index = -1;
				JointNodes.push_back(root);

				// traverse skeleton tree to get all joints
				for (size_t i = 0; i < JointNodes.size(); ++i)
				{
					int childrenCount = JointNodes[i].node->GetChildCount();
					for (int j = 0; j < childrenCount; ++j)
					{
						auto child = JointNodes[i].node->GetChild(j);
						if (child->GetNodeAttribute()
							&& child->GetNodeAttribute()->GetAttributeType()
							&& child->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton
							)
						{
							end::myJoint childJoint;
							childJoint.node = child;
							childJoint.parent_index = i;
							JointNodes.push_back(childJoint);
						}
					}
				}

				// Find mesh
				//FbxMesh* mesh = nullptr;
				//for (int i = 0; i < numItems; ++i)
				//{
				//	FbxNode* node = pose->GetNode(i);
				//	mesh = node->GetMesh();
				//	if (!mesh)
				//		return -1;
				//	else
				//		break; // for now
				//}
				control_point_influences.resize(pMesh->GetControlPointsCount());
				FbxGeometry* geo = (FbxGeometry*)pMesh;
				if (!geo) return -1;

				// find deformer
				int deformerCount = geo->GetDeformerCount(FbxDeformer::EDeformerType::eSkin);

				for (int deformerIndex = 0; deformerIndex < deformerCount; ++deformerIndex)
				{
					FbxDeformer* deformer = geo->GetDeformer(deformerIndex, FbxDeformer::EDeformerType::eSkin);
					if (!deformer) return -1;

					FbxSkin* skin = (FbxSkin*)deformer;
					if (!skin) return -1;

					// get clusters
					int clusterCount = skin->GetClusterCount();
					for (int clusterIndex = 0; clusterIndex < clusterCount; ++clusterIndex)
					{
						FbxCluster* cluster = skin->GetCluster(clusterIndex);
						FbxNode* linkNode = cluster->GetLink();
						size_t jointCount = JointNodes.size();
						size_t jointIndex = 0;

						// find joint index
						for (; jointIndex < jointCount; ++jointIndex)
						{
							if (linkNode == JointNodes[jointIndex].node)
								break;
						}
						// get the influences for the control points
						size_t ctrl_pnt_count = cluster->GetControlPointIndicesCount();
						for (size_t c = 0; c < ctrl_pnt_count; ++c)
						{
							influence inf_to_add;
							inf_to_add.joint = jointIndex;
							inf_to_add.weight = (float)cluster->GetControlPointWeights()[c];
							// potentially doing this wrong 
							influence_set& inf_set = control_point_influences[cluster->GetControlPointIndices()[c]];
							for (size_t inf = 0; inf < MAX_INFLUENCES; ++inf)
							{
								// if this one is stronger than any other kick out the weakest
								if (inf_to_add.weight - inf_set[inf].weight > 0.0001f)
								{
									inf_set[inf] = inf_to_add;
									break;
								}
							}
						}
					}
				}
				#pragma endregion

				// for each polygon of the Mesh
				for (int tri = 0; tri < poly_count; ++tri)
				{
					// for each vertex of the polygon
					for (int v = 0; v < 3; ++v)
					{
						// the current vertex we are writing to
						end::simple_vert out_vert;

						int ctrlPointIndex = pMesh->GetPolygonVertex(tri, v);

						// get pMesh positions
						FbxVector4 position = control_points[ctrlPointIndex];
						out_vert.pos.x = static_cast<float>(position.mData[0]);
						out_vert.pos.y = static_cast<float>(position.mData[1]);
						out_vert.pos.z = static_cast<float>(position.mData[2]);
						out_vert.pos.w = 1.0f;


						#pragma region ANIMATION SKINNING
						influence_set& inf_set = control_point_influences[ctrlPointIndex];
						for (size_t i = 0; i < 4; ++i)
						{
							out_vert.joint_index[i] = inf_set[i].joint;
							out_vert.weights[i] = inf_set[i].weight;
						}
						float sum = out_vert.weights[0] + out_vert.weights[1] + out_vert.weights[2] + out_vert.weights[3];
						for (size_t i = 0; i < 4; ++i)
						{
							out_vert.weights[i] /= sum;
						}
						#pragma	endregion

						// get normals
						ReadNormals(pMesh, output.size(), ctrlPointIndex, out_vert.norm);

						// get pMesh UVs
						ReadUVs(pMesh, output.size(), ctrlPointIndex, out_vert.tex_coord);

						output.push_back(out_vert);
					}
				}
				output.shrink_to_fit();

				end::simple_mesh out_mesh;
				out_mesh.index_count = numIndices;
				out_mesh.indices = (uint32_t*)vert_indices;
				out_mesh.vert_count = 3 * poly_count;
				out_mesh.verts = output.data();

				Compactify(out_mesh);
				for (size_t i = 0; i < out_mesh.vert_count; ++i)
				{
					out_mesh.verts[i].color = { 0.75f, 0.75f, 0.75f, 1.0f };
				}
				Export_Mesh_File(out_mesh, output_file_path);
				delete[] out_mesh.indices;
				delete[] out_mesh.verts;

				result = 0;
			}
		}
		return result;
	}

	void Compactify(end::simple_mesh& simpleMesh)
	{
		std::vector<end::simple_vert> compactedVertexList;
		std::vector<int> indicesList;

		int compactedIndex = 0;
		int const table_size = 1024;
		//std::list<int>* htable = new std::list<int>[table_size];

		for (size_t v = 0; v < simpleMesh.vert_count; ++v)
		{
			int hash = v % table_size;

			bool match = false;
			int index = 0;
			// for each index in the bucket
			for (auto& vert : compactedVertexList/*auto index : htable[hash]*/) // doesn't work
			{
				if (	simpleMesh.verts[v].pos.x == vert.pos.x
					&&	simpleMesh.verts[v].pos.y == vert.pos.y
					&&	simpleMesh.verts[v].pos.z == vert.pos.z
					&&	simpleMesh.verts[v].norm.x == vert.norm.x
					&&	simpleMesh.verts[v].norm.y == vert.norm.y
					&&	simpleMesh.verts[v].norm.z == vert.norm.z
					&&	simpleMesh.verts[v].tex_coord.x == vert.tex_coord.x
					&&	simpleMesh.verts[v].tex_coord.y == vert.tex_coord.y
					)
				{
					indicesList.push_back(index);
					match = true;
					break;
				}
				++index;
			}
			if (!match)
			{
				compactedVertexList.push_back(simpleMesh.verts[v]);
				indicesList.push_back(compactedIndex);
				//htable[hash].push_back(compactedIndex);
				++compactedIndex;
			}
		}

		simpleMesh.indices = nullptr;
		simpleMesh.indices = new uint32_t[indicesList.size()];
		simpleMesh.index_count = static_cast<uint32_t>(indicesList.size());
		memcpy(simpleMesh.indices, indicesList.data(), sizeof(uint32_t) * indicesList.size());

		//delete[] simpleMesh.verts;
		simpleMesh.verts = nullptr;
		simpleMesh.verts = new end::simple_vert[compactedVertexList.size()];
		simpleMesh.vert_count = static_cast<uint32_t>(compactedVertexList.size());
		memcpy(simpleMesh.verts, compactedVertexList.data(), sizeof(end::simple_vert) * compactedVertexList.size());

		//delete[] htable;
	}

	int Process_Animation(const char* output_file_path)
	{
		//int result = -1;
		// find bind pose
		int pose_count = FBXUtils::Scene->GetPoseCount();
		FbxPose* pose = nullptr;
		for (int i = 0; i < pose_count; ++i)
		{
			pose = FBXUtils::Scene->GetPose(i);
			if (pose->IsBindPose())
				break;
		}
		if (!pose || !pose->IsBindPose())
			return -1;

		// find Skeleton
		int numItems = pose->GetCount();
		FbxSkeleton* skeleton = nullptr;
		for (int i = 0; i < numItems; ++i)
		{
			skeleton = pose->GetNode(i)->GetSkeleton();
			if (skeleton && skeleton->IsSkeletonRoot())
				break;
		}
		if (!skeleton || !skeleton->IsSkeletonRoot())
			return -1;

		std::vector<end::myJoint> JointNodes;

		end::myJoint root;
		root.node = skeleton->GetNode(0);
		root.parent_index = -1;
		JointNodes.push_back(root);

		// traverse skeleton tree to get all joints
		for (size_t i = 0; i < JointNodes.size(); ++i)
		{
			int childrenCount = JointNodes[i].node->GetChildCount();
			for (int j = 0; j < childrenCount; ++j)
			{
				auto child = JointNodes[i].node->GetChild(j);
				if (child->GetNodeAttribute()
					&& child->GetNodeAttribute()->GetAttributeType()
					&& child->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton
					)
				{
					end::myJoint childJoint;
					childJoint.node = child;
					childJoint.parent_index = i;
					JointNodes.push_back(childJoint);
				}
			}
		}

		FbxAnimStack* currStack = FBXUtils::Scene->GetCurrentAnimationStack();
		FbxTimeSpan timeSpan = currStack->GetLocalTimeSpan();
		FbxTime timeStart = timeSpan.GetStart();
		FbxTime timeEnd = timeSpan.GetStop();

		end::AnimClip clip;
		clip.duration = static_cast<double>(timeEnd.GetFrameCount(FbxTime::eFrames24) - timeStart.GetFrameCount(FbxTime::eFrames24) + 1);
		// for every frame in the animation
		for (size_t i = timeStart.GetFrameCount(FbxTime::eFrames24); i < timeEnd.GetFrameCount(FbxTime::eFrames24); ++i)
		{
			end::myKeyFrame keyframe;
			FbxTime currTime;
			currTime.SetFrame(i, FbxTime::eFrames24);
			keyframe.time = currTime.GetSecondDouble();

			// get transform for every joint in the keyframe
			for (auto& joint : JointNodes)
			{
				FbxAMatrix currTransformOffset = joint.node->EvaluateGlobalTransform(currTime);
				end::Joint newJoint;
				newJoint.global_xform = {
					(float)currTransformOffset.Get(0, 0), (float)currTransformOffset.Get(0, 1), (float)currTransformOffset.Get(0, 2), (float)currTransformOffset.Get(0, 3),
					(float)currTransformOffset.Get(1, 0), (float)currTransformOffset.Get(1, 1), (float)currTransformOffset.Get(1, 2), (float)currTransformOffset.Get(1, 3),
					(float)currTransformOffset.Get(2, 0), (float)currTransformOffset.Get(2, 1), (float)currTransformOffset.Get(2, 2), (float)currTransformOffset.Get(2, 3),
					(float)currTransformOffset.Get(3, 0), (float)currTransformOffset.Get(3, 1), (float)currTransformOffset.Get(3, 2), (float)currTransformOffset.Get(3, 3)
				};
				newJoint.parent_index = joint.parent_index;
				keyframe.joints.push_back(newJoint);
			}
			clip.frames.push_back(keyframe);
		}
		clip.frameCount = clip.frames.size();
#		pragma region ANIMATION SKINNING
		// Animation Skinning===========================================
		const int MAX_INFLUENCES = 4; // max number of joints influencing a single vertex
		struct influence { int joint; float weight; };
		using influence_set = std::array<influence, MAX_INFLUENCES>;
		std::vector<influence_set> control_point_influences;

		// Find mesh
		FbxMesh* mesh = nullptr;
		for (int i = 0; i < numItems; ++i)
		{
			FbxNode* node = pose->GetNode(i);
			mesh = node->GetMesh();
			if (!mesh)
				return -1;
			else
				break; // for now
		}
		control_point_influences.resize(mesh->GetControlPointsCount());
		FbxGeometry* geo = (FbxGeometry*)mesh;
		if (!geo) return -1;

		// find deformer
		int deformerCount = geo->GetDeformerCount(FbxDeformer::EDeformerType::eSkin);

		for (int deformerIndex = 0; deformerIndex < deformerCount; ++deformerIndex)
		{
			FbxDeformer* deformer = geo->GetDeformer(deformerIndex, FbxDeformer::EDeformerType::eSkin);
			if (!deformer) return -1;

			FbxSkin* skin = (FbxSkin*)deformer;
			if (!skin) return -1;

			// get clusters
			int clusterCount = skin->GetClusterCount();
			for (int clusterIndex = 0; clusterIndex < clusterCount; ++clusterIndex)
			{
				FbxCluster* cluster = skin->GetCluster(clusterIndex);
				FbxNode* linkNode = cluster->GetLink();
				size_t jointCount = JointNodes.size();
				size_t jointIndex = 0;

				//// Attempt 1 at globalbindpose inverse===========================
				//FbxAMatrix transformMatrix; 
				//FbxAMatrix transformLinkMatrix;
				//FbxAMatrix globalBindposeInverseMatrix;
				//cluster->GetTransformMatrix(transformMatrix);
				//// The transformation of the mesh at binding time 
				//cluster->GetTransformLinkMatrix(transformLinkMatrix);
				//// The transformation of the cluster(joint) at binding time from joint space to world space 
				//globalBindposeInverseMatrix = transformLinkMatrix.Inverse() * transformMatrix;
				////===============================================================

				// find joint index
				for (; jointIndex < jointCount; ++jointIndex)
				{
					if (linkNode == JointNodes[jointIndex].node)
					{
						//for (size_t i = 0; i < clip.frameCount; ++i)
						//{
						//	clip.frames[i].joints[jointIndex].inverse_xform = {
						//		(float)globalBindposeInverseMatrix.Get(0, 0), (float)globalBindposeInverseMatrix.Get(0, 1), (float)globalBindposeInverseMatrix.Get(0, 2), (float)globalBindposeInverseMatrix.Get(0, 3),
						//		(float)globalBindposeInverseMatrix.Get(1, 0), (float)globalBindposeInverseMatrix.Get(1, 1), (float)globalBindposeInverseMatrix.Get(1, 2), (float)globalBindposeInverseMatrix.Get(1, 3),
						//		(float)globalBindposeInverseMatrix.Get(2, 0), (float)globalBindposeInverseMatrix.Get(2, 1), (float)globalBindposeInverseMatrix.Get(2, 2), (float)globalBindposeInverseMatrix.Get(2, 3),
						//		(float)globalBindposeInverseMatrix.Get(3, 0), (float)globalBindposeInverseMatrix.Get(3, 1), (float)globalBindposeInverseMatrix.Get(3, 2), (float)globalBindposeInverseMatrix.Get(3, 3)
						//	};
						//}
						break;
					}
						
				}

				// get the influences for the control points
				size_t ctrl_pnt_count = cluster->GetControlPointIndicesCount();
				for (size_t c = 0; c < ctrl_pnt_count; ++c)
				{
					influence inf_to_add;
					inf_to_add.joint = jointIndex;
					inf_to_add.weight = (float)cluster->GetControlPointWeights()[c];
					// potentially doing this wrong 
					influence_set& inf_set = control_point_influences[cluster->GetControlPointIndices()[c]];
					for (size_t inf = 0; inf < MAX_INFLUENCES; ++inf)
					{
						// if this one is stronger than any other kick out the weakest
						if (inf_to_add.weight - inf_set[inf].weight > 0.0001f)
						{
							inf_set[inf] = inf_to_add;
							break;
						}
					}
				}
			}
		}
		#pragma endregion

		Export_Animation_File(&clip, output_file_path);

		return 0;
	}

	//	3.Write the 'simple_mesh' object data to a binary file using 'output_file_path'
	void Export_Mesh_File(end::simple_mesh mesh, const char* output_file_path)
	{
		std::ofstream file(output_file_path, std::ios::trunc | std::ios::binary | std::ios::out);

		assert(file.is_open());

		if (file.is_open())
		{
			file.write((char const*)&mesh.index_count, sizeof(uint32_t));
			file.write((char const*)mesh.indices, sizeof(uint32_t) * mesh.index_count);
			file.write((char const*)&mesh.vert_count, sizeof(uint32_t));
			file.write((char const*)mesh.verts, sizeof(end::simple_vert) * mesh.vert_count);
		}

		file.close();
	}

	void Export_Animation_File(end::AnimClip* animClip, const char* output_file_path)
	{
		std::ofstream file(output_file_path, std::ios::trunc | std::ios::binary | std::ios::out);

		assert(file.is_open());

		if (file.is_open())
		{
			file.write((char const*)&animClip->duration, sizeof(double));
			animClip->frameCount = animClip->frames.size();
			file.write((char const*)&animClip->frameCount, sizeof(int));
			//file.write((char const*)animClip->frames.data(), sizeof(end::myKeyFrame) * animClip->frames.size());

			for (size_t i = 0; i < animClip->frameCount; ++i)
			{
				file.write((char const*)&animClip->frames[i].time, sizeof(double));
				file.write((char const*)animClip->frames[i].joints.data(), sizeof(end::Joint) * animClip->frames[i].joints.size());
			}
		}

		file.close();
	}
}

int Get_Scene_Poly_Count(const char* fbx_file_path)
{
	int result = -1;
	// Scene pointer, set by call to create_and_import
	FbxScene* scene = nullptr;
	// Create the FbxManager and import the scene from file
	FbxManager* sdk_manager = FBXUtils::Create_and_Import(fbx_file_path, scene);
	// Check if manager creation failed
	if (sdk_manager == nullptr)
		return result;
	//If the scene was imported...
	if (scene != nullptr)
	{
		//No errors to report, so start polygon count at 0
		result = 0;
		// Get the count of geometry objects in the scene
		int geo_count = scene->GetGeometryCount();
		for (int i = 0; i < geo_count; ++i)
		{
			//Get geometry number 'i'
			FbxGeometry* geo = scene->GetGeometry(i);
			// If it's not a mesh, skip it
			// Geometries might be some other type like nurbs
			if (geo->GetAttributeType() != FbxNodeAttribute::eMesh)
				continue;
			// Found a mesh, add its polygon count to the result
			FbxMesh* mesh = (FbxMesh*)geo;
			result += mesh->GetPolygonCount();
		}
	}
	//Destroy the manager
	sdk_manager->Destroy();
	//Return the polygon count for the scene
	return result;
}

//	1.Searches the scene for a mesh with 'mesh_name'
//		a. If 'mesh_name' is null, proceed by using the first mesh in the scene
//		b. if no match is found, return an integer error code (ex: -1)
int export_simple_mesh(const char* fbx_file_path, const char* output_file_path, const char* mesh_name)
{
	int result = -1;
	// Scene pointer, set by call to create_and_import
	FBXUtils::Scene = nullptr;
	// Create the FbxManager and import the scene from file
	FBXUtils::sdk_manager = FBXUtils::Create_and_Import(fbx_file_path, FBXUtils::Scene);
	// Check if manager creation failed
	if (FBXUtils::sdk_manager == nullptr)
		return result;
	//If the scene was imported...
	if (FBXUtils::Scene != nullptr)
	{
		if (mesh_name != nullptr)
		{
			int node_count = FBXUtils::Scene->GetNodeCount();
			for (int i = 0; i < node_count; ++i)
			{
				FbxNode* node = FBXUtils::Scene->GetNode(i);
				if (mesh_name == node->GetMesh()->GetName())
				{
					result = FBXUtils::Process_Mesh(node, output_file_path);
				}
			}
		}
		else
			result = FBXUtils::Process_Mesh(FBXUtils::Scene->GetRootNode(), output_file_path);
	}
	//Destroy the manager
	FBXUtils::sdk_manager->Destroy();

	return result;
}

int export_materials(const char* fbx_file_path, const char* output_file_path)
{
	int result = -1;
	// Scene pointer, set by call to create_and_import
	FBXUtils::Scene = nullptr;
	// Create the FbxManager and import the scene from file
	FBXUtils::sdk_manager = FBXUtils::Create_and_Import(fbx_file_path, FBXUtils::Scene);
	// Check if manager creation failed
	if (FBXUtils::sdk_manager == nullptr)
		return result;
	//If the scene was imported...
	if (FBXUtils::Scene != nullptr)
	{
		std::vector <end::material_t> output_mats;
		std::vector<std::array<char, 260>> paths;

		int num_mats = FBXUtils::Scene->GetMaterialCount();

		for (int m = 0; m < num_mats; ++m)
		{
			end::material_t out_mat;
			FbxSurfaceMaterial* mat = FBXUtils::Scene->GetMaterial(m);

			if (mat->Is<FbxSurfaceLambert>() == false) // non-standard material, skip for now
				continue;

			// get Diffuse
			FbxSurfaceLambert* lam = (FbxSurfaceLambert*)mat;
			FbxDouble3 diffuse_color = lam->Diffuse.Get();
			FbxDouble diffuse_factor = lam->DiffuseFactor.Get();
			out_mat[out_mat.DIFFUSE].value[0] = static_cast<float>(diffuse_color[0]);
			out_mat[out_mat.DIFFUSE].value[1] = static_cast<float>(diffuse_color[1]);
			out_mat[out_mat.DIFFUSE].value[2] = static_cast<float>(diffuse_color[2]);
			out_mat[out_mat.DIFFUSE].factor = static_cast<float>(diffuse_factor);
			if (FbxFileTexture* file_texture = lam->Diffuse.GetSrcObject<FbxFileTexture>())
			{
				char const* file_name = file_texture->GetRelativeFileName();
				std::array<char, 260> file_path;
				strcpy_s(file_path.data(), sizeof(file_path), file_name);
				out_mat[out_mat.DIFFUSE].input = paths.size();
				paths.push_back(file_path);
				result = 0;
			}

			// get Emissive
			//FbxSurfaceLambert* lam = (FbxSurfaceLambert*)mat;
			FbxDouble3 emissive_color = lam->Emissive.Get();
			FbxDouble emissive_factor = lam->EmissiveFactor.Get();
			out_mat[out_mat.EMISSIVE].value[0] = static_cast<float>(emissive_color[0]);
			out_mat[out_mat.EMISSIVE].value[1] = static_cast<float>(emissive_color[1]);
			out_mat[out_mat.EMISSIVE].value[2] = static_cast<float>(emissive_color[2]);
			out_mat[out_mat.EMISSIVE].factor = static_cast<float>(emissive_factor);
			if (FbxFileTexture* file_texture = lam->Emissive.GetSrcObject<FbxFileTexture>())
			{
				char const* file_name = file_texture->GetRelativeFileName();
				std::array<char, 260> file_path;
				strcpy_s(file_path.data(), sizeof(file_path), file_name);
				out_mat[out_mat.EMISSIVE].input = paths.size();
				paths.push_back(file_path);
				result = 0;
			}

			// get Specular
			if (mat->Is<FbxSurfacePhong>())
			{
				FbxSurfacePhong* spec = (FbxSurfacePhong*)mat;
				FbxDouble3 spec_color = spec->Specular.Get();
				FbxDouble spec_factor = spec->SpecularFactor.Get();
				out_mat[out_mat.SPECULAR].value[0] = static_cast<float>(spec_color[0]);
				out_mat[out_mat.SPECULAR].value[1] = static_cast<float>(spec_color[1]);
				out_mat[out_mat.SPECULAR].value[2] = static_cast<float>(spec_color[2]);
				out_mat[out_mat.SPECULAR].factor = static_cast<float>(spec_factor);
				if (FbxFileTexture* file_texture = spec->Specular.GetSrcObject<FbxFileTexture>())
				{
					char const* file_name = file_texture->GetRelativeFileName();
					std::array<char, 260> file_path;
					strcpy_s(file_path.data(), sizeof(file_path), file_name);
					out_mat[out_mat.SPECULAR].input = paths.size();
					paths.push_back(file_path);
					result = 0;
				}
			}

			output_mats.push_back(out_mat);
		}

		// write Material data to .mat file
		std::ofstream file(output_file_path, std::ios::trunc | std::ios::binary | std::ios::out);

		//assert(file.is_open());
		if (file.is_open())
		{
			size_t mat_count = output_mats.size();
			size_t path_count = paths.size();
			file.write((char const*)&mat_count, sizeof(size_t));
			file.write((char const*)output_mats.data(), sizeof(end::material_t) * mat_count);
			file.write((char const*)&path_count, sizeof(size_t));
			file.write((char const*)paths.data(), sizeof(std::array<char, 260>) * path_count);
		}

		file.close();
	}
	//Destroy the manager
	FBXUtils::sdk_manager->Destroy();

	return result;
}

int export_animation(const char* fbx_file_path, const char* output_file_path)
{
	int result = -1;
	// Scene pointer, set by call to create_and_import
	FBXUtils::Scene = nullptr;
	// Create the FbxManager and import the scene from file
	FBXUtils::sdk_manager = FBXUtils::Create_and_Import(fbx_file_path, FBXUtils::Scene);
	// Check if manager creation failed
	if (FBXUtils::sdk_manager == nullptr)
		return result;
	//If the scene was imported...
	if (FBXUtils::Scene != nullptr)
	{
		result = FBXUtils::Process_Animation(output_file_path);
	}
	//Destroy the manager
	FBXUtils::sdk_manager->Destroy();

	return result;
}
