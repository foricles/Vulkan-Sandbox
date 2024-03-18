#pragma once
#include <vulkan/vulkan.h>
#include "../vkcommon.hpp"
#include "../vktexture.hpp"
#include "../vkbuffer.hpp"
#include "../vkrenderpass.hpp"
#include <array>
#include <optional>
#include <functional>
#include <string_view>
#include <unordered_map>


class ShaderBinder
{
public:
	ShaderBinder(VkDescriptorSet descriptorSet);

	ShaderBinder& ImageSampler(const Sampler& sampler, uint32_t location);
	ShaderBinder& Image(const Texture& texture, uint32_t location, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	ShaderBinder& StorageImage(const Texture& texture, uint32_t location, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_GENERAL);
	ShaderBinder& UniformBuffer(const Buffer& buffer, uint32_t location);
	ShaderBinder& StorageBuffer(const Buffer& buffer, uint32_t location);
	ShaderBinder& StorageBufferReadonly(const Buffer& buffer, uint32_t location);
	ShaderBinder& AccelerationStructure(const VkAccelerationStructureKHR& structure, uint32_t location);

	void Bind();

private:
	VkDescriptorSet descriptorSet;
	std::vector<VkDescriptorImageInfo> images;
	std::vector<VkDescriptorBufferInfo> buffers;
	std::vector<VkAccelerationStructureKHR> accelerationStructures;
	std::vector<VkWriteDescriptorSetAccelerationStructureKHR> accelerationStructuresWrites;
	std::vector<VkWriteDescriptorSet> writes;
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

		static inline ShaderBinder Binder() { return ShaderBinder(vkDesciptorSet); }
	private:
		static uint32_t counter;
	};

protected:
	struct PipeStateObj
	{
		VkPipeline vkPipeline{ VK_NULL_HANDLE };
		VkPipelineLayout vkPipelineLayout{ VK_NULL_HANDLE };
	};

	struct VulkanShader
	{
		std::unordered_map<uint32_t, VkDescriptorSet> perDrawcallDescriptorSets;		// key - descriptor set mask
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		VkDescriptorSetLayout vkPerDrawcallDesciptorSetLayout{ VK_NULL_HANDLE };

	public:
		template<class T> inline T& GetPso()
		{
			return *reinterpret_cast<T*>(pipelineStateObj + 1);
		}

		template<class T> inline T& NewPso()
		{
			static_assert(sizeof(T) < sizeof(pipelineStateObj));
			new (pipelineStateObj + 1) T();
			pipelineStateObj[0] = 0xFF;

			return GetPso<T>();
		}

		inline bool IsPsoNull()
		{
			return pipelineStateObj[0] == 0;
		}

	private:
		uint8_t pipelineStateObj[256]{ 0 };
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

public:
	Shader();
	virtual ~Shader();

	void SetSource(std::string_view source);
	void MarkProgram(EShaderType eType, std::string_view entry);

	virtual void Bind(VkCommandBuffer commandBuffer) = 0;

	inline ShaderBinder Binder() { return ShaderBinder(m_descriptorSetCache); }
	inline bool HasBindables() const { return m_descriptorSetCache != VK_NULL_HANDLE; }

protected:
	VulkanShader& CompileStages(uint64_t bitmask);
	void CreatePipelineLayout(VulkanShader& shader, PipeStateObj& pipelineStateObject);
	void CreatePerDrawcallDescriptorSet(VulkanShader& shader, uint32_t descriptorSetMask);

	void ReflectShaderProgram(ShaderProgram& shaderProgram);
	std::optional<ShaderProgram> CompileShaderProgram(const std::string_view mainName, EShaderType eType, const std::vector<std::string> defines = {});

protected:
	VkDescriptorSet m_descriptorSetCache;
	std::string m_source;
	std::unordered_map<uint64_t, VulkanShader> m_ShaderVariant;
	std::vector<std::pair<EShaderType, std::string>> m_stages;
};