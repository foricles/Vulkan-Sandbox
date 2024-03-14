#include "computeshader.hpp"


ShaderCompute::ShaderCompute()
{
}

ShaderCompute::~ShaderCompute()
{
	for (auto& shader : m_ShaderVariant)
	{
		auto& pipelineStateObject = shader.second.GetPso<PipeStateObj>();
		vkDestroyPipelineLayout(VkGlobals::vkDevice, pipelineStateObject.vkPipelineLayout, VkGlobals::vkAllocatorCallback);
		vkDestroyPipeline(VkGlobals::vkDevice, pipelineStateObject.vkPipeline, VkGlobals::vkAllocatorCallback);
		pipelineStateObject.~PipeStateObj();
	}
}

void ShaderCompute::Bind(VkCommandBuffer commandBuffer)
{
	std::vector<VkDescriptorSet> descriptorSets = { Shader::PerFrameDescriptors::vkDesciptorSet };
	if (m_descriptorSetCache != VK_NULL_HANDLE)
	{
		descriptorSets.push_back(m_descriptorSetCache);
	}

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		m_psoCache.vkPipelineLayout,
		0,
		uint32_t(descriptorSets.size()),
		descriptorSets.data(),
		0, nullptr
	);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_psoCache.vkPipeline);
}

void ShaderCompute::SetState(uint64_t bitmask, uint32_t descriptorSetMask)
{
	VulkanShader& shader = CompileStages(bitmask);
	CreatePerDrawcallDescriptorSet(shader, descriptorSetMask);

	if (shader.IsPsoNull())
	{
		auto& pipelineStateObject = shader.NewPso<PipeStateObj>();
		CreatePipelineLayout(shader, pipelineStateObject);
		CreateComputePso(shader, pipelineStateObject);
		m_psoCache = pipelineStateObject;
	}
	else
	{
		m_psoCache = shader.GetPso<PipeStateObj>();
	}
}


void ShaderCompute::CreateComputePso(VulkanShader& shader, PipeStateObj& pipelineStateObject)
{
	VkComputePipelineCreateInfo computeInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	computeInfo.stage = shader.shaderStages[0];
	computeInfo.layout = pipelineStateObject.vkPipelineLayout;
	VK_ASSERT(vkCreateComputePipelines(VkGlobals::vkDevice, VK_NULL_HANDLE, 1, &computeInfo, VkGlobals::vkAllocatorCallback, &pipelineStateObject.vkPipeline));
}