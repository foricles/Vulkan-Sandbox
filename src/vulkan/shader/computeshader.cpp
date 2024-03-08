#include "computeshader.hpp"


ShaderCompute::ShaderCompute()
{
}

ShaderCompute::~ShaderCompute()
{
	for (auto& psoPair : m_pipelineStateObjects)
	{
		vkDestroyPipelineLayout(VkGlobals::vkDevice, psoPair.second.vkPipelineLayout, VkGlobals::vkAllocatorCallback);
		vkDestroyPipeline(VkGlobals::vkDevice, psoPair.second.vkPipeline, VkGlobals::vkAllocatorCallback);
	}
}

void ShaderCompute::Bind(VkCommandBuffer commandBuffer)
{
	std::vector<VkDescriptorSet> descriptorSets = { Shader::PerFrameDescriptors::vkDesciptorSet };
	if (m_descriptorSetCache != nullptr)
	{
		descriptorSets.push_back(*m_descriptorSetCache);
	}

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		m_psoCache->vkPipelineLayout,
		0,
		uint32_t(descriptorSets.size()),
		descriptorSets.data(),
		0, nullptr
	);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_psoCache->vkPipeline);
}

void ShaderCompute::SetState(uint64_t bitmask, uint32_t descriptorSetMask)
{
	VulkanShader& shader = CompileStages(bitmask);
	CreatePerDrawcallDescriptorSet(shader, descriptorSetMask);

	auto& fnd = m_pipelineStateObjects.find(bitmask);
	if (fnd == m_pipelineStateObjects.end())
	{
		PipeStateObj& pipelineStateObject = m_pipelineStateObjects[bitmask];
		CreateComputePso(shader, pipelineStateObject);
	}
	else
	{
		m_psoCache = &fnd->second;
	}
}


void ShaderCompute::CreateComputePso(VulkanShader& shader, PipeStateObj& pipelineStateObject)
{
	VkComputePipelineCreateInfo computeInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	computeInfo.stage = shader.shaderStages[0];
	computeInfo.layout = pipelineStateObject.vkPipelineLayout;
	VK_ASSERT(vkCreateComputePipelines(VkGlobals::vkDevice, VK_NULL_HANDLE, 1, &computeInfo, VkGlobals::vkAllocatorCallback, &pipelineStateObject.vkPipeline));
}