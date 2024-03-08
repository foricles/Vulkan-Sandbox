#pragma once
#include "vkshader.hpp"


class ShaderCompute : public Shader
{
public:
	ShaderCompute();
	~ShaderCompute();

	void Bind(VkCommandBuffer commandBuffer) override;

	void SetState(uint64_t bitmask, uint32_t descriptorSetMask);

private:
	void CreateComputePso(VulkanShader& shader, PipeStateObj& pipelineStateObject);

private:
	std::unordered_map<uint64_t, PipeStateObj> m_pipelineStateObjects;
};