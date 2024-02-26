#pragma once
#include <vector>
#include <string_view>
#include <array>
#include <unordered_map>
#include "../vulkan/vkcommon.hpp"


struct Mesh
{
	uint32_t materialId{ UINT32_MAX };
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

struct RawTexture
{
	uint32_t width{ 0 };
	uint32_t height{ 0 };
	uint32_t chanels{ 0 };
	std::vector<uint8_t> data;

	bool IsValid() const { return width > 0 && height > 0 && chanels > 0; }
};

struct Material
{
	RawTexture diffuseTexture;
	RawTexture normalTexture;
};

class Model
{
public:
	std::vector<Mesh> meshes;
	std::unordered_map<uint32_t, Material> materials;
};

class ModelLoader
{
public:
	Model Load(std::string_view pilepath);
};