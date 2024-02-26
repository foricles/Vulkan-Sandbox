#pragma once
#include <vulkan/vulkan.h>
#include "vkcommon.hpp"
#include "vktexture.hpp"
#include "vkbuffer.hpp"
#include "vkrenderpass.hpp"
#include <array>
#include <optional>
#include <functional>
#include <string_view>
#include <unordered_map>


struct RenderState
{
	ECull cullMode;
	ETopology topology;
	EFillMode fillMode;
	EFrontFace frontFace;
	EDepthFunc depthFunc;
	bool hasInputAttachment;
	bool isCompute;

	RenderState();
	uint64_t Hash() const;
};


struct ShaderProgram
{
	struct Descriptor
	{
		uint32_t size;
		uint32_t binding;
		EDescriptorType type;
	};

	std::vector<uint8_t> spirv;
	std::vector<Descriptor> perDrawcallDescriptos;
};


class Shader
{
public:
	struct PerFrameDescriptors
	{
		friend class Shader;
	public:
		static VkDescriptorSet vkDesciptorSet;
		static VkDescriptorSetLayout vkDesciptorSetLayout;
	private:
		static uint32_t counter;
	};

private:
	struct PipeStateObj
	{
		VkPipeline vkPipeline{ VK_NULL_HANDLE };
		VkPipelineLayout vkPipelineLayout{ VK_NULL_HANDLE };
	};

	struct BitmaskFilter
	{
		std::unordered_map<uint64_t, PipeStateObj> m_PSOs;								// key - render state
		std::unordered_map<uint32_t, VkDescriptorSet> m_PerDrawcallDescriptorSets;		// key - descriptor set mask
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		VkDescriptorSetLayout vkPerDrawcallDesciptorSetLayout{ VK_NULL_HANDLE };
	};

public:
	Shader();
	Shader(std::string_view source);
	~Shader();

	void SetSource(std::string_view source);
	void MarkProgram(EShaderType eType, std::string_view entry);
	void SetState(const Renderpass& renderpass, uint64_t bitmask, uint32_t descriptorSetMask, const RenderState& renderState);
	void SetComputeState(uint64_t bitmask, uint32_t descriptorSetMask);

	PipeStateObj& GetPipeStateObj();
	VkDescriptorSet& GetPerDrawCallSet();

	void WriteSampler(const Sampler& sampler, uint32_t location);
	void WriteImage(const Texture& texture, uint32_t location, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	void WriteStorageImage(const Texture& texture, uint32_t location, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	void WriteStorageBuffer(const Buffer& buffer, uint32_t location);
	void WriteStorageBufferReadonly(const Buffer& buffer, uint32_t location);

	static void WritePerFrameBuffer(const Buffer& buffer, uint32_t location);

	const bool HasPerDrawcallSet() const { return  m_descriptorSetCache != nullptr; }

private:
	void CompileStages(uint64_t bitmask);
	PipeStateObj& CreatePso(uint64_t bitmask, const Renderpass& renderpass, const RenderState& renderState);
	void CreateGraphicsPso(BitmaskFilter& shaderVariant, PipeStateObj& pso, const Renderpass& renderpass, const RenderState& renderState);
	void CreateComputePso(BitmaskFilter& shaderVariant, PipeStateObj& pso);
	VkDescriptorSet& CreatePerDrawcallDescriptorSet(uint64_t bitmask, uint32_t descriptorSetMask);

	void ReflectShaderProgram(ShaderProgram& shaderProgram);
	std::optional<ShaderProgram> CompileShaderProgram(const std::string_view mainName, EShaderType eType, const std::vector<std::string> defines = {});

private:
	PipeStateObj* m_psoCache;
	VkDescriptorSet* m_descriptorSetCache;
	std::array<std::string, (uint32_t)EShaderType::COUNT> m_stages;
	std::string m_source;
	std::unordered_map<uint64_t, BitmaskFilter> m_ShaderVariant;
};