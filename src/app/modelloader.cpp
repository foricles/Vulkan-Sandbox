#include "modelloader.hpp"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/matrix4x4.h>
#include <assimp/matrix4x4.inl>
#include <assimp/quaternion.h>
#include <assimp/quaternion.inl>
#include <stack>
#include <filesystem>
#define STB_IMAGE_IMPLEMENTATION
#include <stbi/stb_image.h>




inline void ReadMeshFromNode(aiMesh* pMesh, const aiMatrix4x4& mat, Mesh& mesh)
{
	mesh.vertices.resize(pMesh->mNumVertices);
	mesh.materialId = pMesh->mMaterialIndex;
	for (uint32_t i(0); i < pMesh->mNumVertices; i++)
	{

		aiVector3D temp;
		aiQuaterniont<ai_real> quat;
		mat.DecomposeNoScaling(quat, temp);
		const aiVector3D pPos = mat * pMesh->mVertices[i];
		const aiVector3D& pNormal = quat.Rotate(pMesh->mNormals[i]).Normalize();
		const aiVector3D& pTangent = quat.Rotate(pMesh->mTangents[i]).Normalize();
		const aiVector3D& pBitangents = quat.Rotate(pMesh->mBitangents[i]).Normalize();

		mesh.vertices[i].uv = 0;
		mesh.vertices[i].uw = 0;
		if (pMesh->HasTextureCoords(0))
		{
			const aiVector3D* pTexCoord1 = &(pMesh->mTextureCoords[0][i]);
			mesh.vertices[i].uv = pTexCoord1->x;
			mesh.vertices[i].uw = pTexCoord1->y;
		}

		mesh.vertices[i].px = pPos.x;
		mesh.vertices[i].py = pPos.y;
		mesh.vertices[i].pz = pPos.z;
		mesh.vertices[i].nx = pNormal.x;
		mesh.vertices[i].ny = pNormal.y;
		mesh.vertices[i].nz = pNormal.z;
		mesh.vertices[i].tx = pTangent.x;
		mesh.vertices[i].ty = pTangent.y;
		mesh.vertices[i].tz = pTangent.z;
		mesh.vertices[i].bx = pBitangents.x;
		mesh.vertices[i].by = pBitangents.y;
		mesh.vertices[i].bz = pBitangents.z;
	}

	mesh.indices.reserve(pMesh->mNumFaces * 3);
	for (unsigned int k = 0; k < pMesh->mNumFaces; k++)
	{
		const aiFace& Face = pMesh->mFaces[k];
		mesh.indices.push_back(Face.mIndices[0]);
		mesh.indices.push_back(Face.mIndices[1]);
		mesh.indices.push_back(Face.mIndices[2]);
	}
}
#include <iostream>
inline void LoadTexture(RawTexture& texture, std::string_view path)
{
	int width{ 0 }, height{ 0 }, chanels{ 0 };
	if (1 == stbi_info(path.data(), &width, &height, &chanels))
	{
		if (width == 0)
		{
			int a = 0;
		}
		int reqChanels = chanels;
		if (chanels == 3)
		{
			reqChanels = 4;
		}

		texture.width = uint32_t(width);
		texture.height = uint32_t(height);
		texture.chanels = uint32_t(reqChanels);

		stbi_uc* rawbuff = stbi_load(path.data(), &width, &height, &chanels, reqChanels);

		const uint32_t imSize = width * height * reqChanels * sizeof(uint8_t);
		texture.data.resize(imSize);
		::memcpy(&texture.data[0], rawbuff, imSize);

		stbi_image_free(rawbuff);
	}
	else
	{
		std::cout << "\tCan not find texture: " << path << std::endl;
		stbi__err(NULL, NULL);
	}
}

inline void ReadMaterial(Material& material, const std::filesystem::path& path, const aiScene* pScene, int32_t matId)
{
	aiMaterial* pMaterial = pScene->mMaterials[matId];

	if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
	{
		aiString texturePath;
		if (AI_SUCCESS == pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath))
		{
			pMaterial->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), texturePath);

			std::string parentPath = (path / std::filesystem::path(texturePath.C_Str()).filename()).string();

			LoadTexture(material.diffuseTexture, (path / std::filesystem::path(texturePath.C_Str()).filename()).string());
		}
	}
	else if (pMaterial->GetTextureCount(aiTextureType_BASE_COLOR) > 0)
	{
		aiString texturePath;
		if (AI_SUCCESS == pMaterial->GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath))
		{
			pMaterial->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), texturePath);

			std::string parentPath = (path / std::filesystem::path(texturePath.C_Str()).filename()).string();

			LoadTexture(material.diffuseTexture, (path / std::filesystem::path(texturePath.C_Str()).filename()).string());
		}
	}

	if (pMaterial->GetTextureCount(aiTextureType_NORMALS) > 0)
	{
		aiString texturePath;
		if (AI_SUCCESS == pMaterial->GetTexture(aiTextureType_NORMALS, 0, &texturePath))
		{
			pMaterial->Get(AI_MATKEY_TEXTURE_NORMALS(0), texturePath);

			std::string parentPath = (path / std::filesystem::path(texturePath.C_Str()).filename()).string();

			LoadTexture(material.normalTexture, (path / std::filesystem::path(texturePath.C_Str()).filename()).string());
		}
	}
}

Model ModelLoader::Load(std::string_view pilepath)
{
	const std::filesystem::path texturePath = std::filesystem::path(pilepath.data()).parent_path() / "textures";

	Assimp::Importer Importer;
	const auto flags = aiProcess_FlipUVs
		| aiProcess_Triangulate
		| aiProcess_GenSmoothNormals
		| aiProcess_EmbedTextures
		| aiProcess_CalcTangentSpace;
	const aiScene* pScene = Importer.ReadFile(pilepath.data(), flags);

	Model model;

	if (pScene && (pScene->mNumMeshes > 0))
	{
		std::stack<aiNode*, std::vector<aiNode*>> nodeStack;
		std::stack<aiMatrix4x4, std::vector<aiMatrix4x4>> matrixStack;

		model.meshes.reserve(pScene->mNumMeshes);

		nodeStack.push(pScene->mRootNode);
		matrixStack.push(pScene->mRootNode->mTransformation);

		while (!nodeStack.empty())
		{
			const aiMatrix4x4 nodeMtx = matrixStack.top();
			matrixStack.pop();
			const aiNode* pNode = nodeStack.top();
			nodeStack.pop();

			if (uint32_t uNumMeshes = pNode->mNumMeshes)
			{
				for (uint32_t i(0); i < pNode->mNumMeshes; ++i)
				{
					Mesh mesh;
					ReadMeshFromNode(pScene->mMeshes[pNode->mMeshes[i]], nodeMtx, mesh);

					if (model.materials.find(mesh.materialId) == model.materials.end())
					{
						Material material;
						ReadMaterial(material, texturePath, pScene, mesh.materialId);
						model.materials[mesh.materialId] = std::move(material);
					}

					model.meshes.push_back(std::move(mesh));
				}
			}

			if (uint32_t uNumChildren = pNode->mNumChildren)
			{
				for (uint32_t i(0); i < uNumChildren; ++i)
				{
					nodeStack.push(pNode->mChildren[i]);
					matrixStack.push(nodeMtx * pNode->mChildren[i]->mTransformation);
				}
			}
		}

		model.meshes.shrink_to_fit();
	}

	return model;
}