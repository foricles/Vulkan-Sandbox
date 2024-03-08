#include "graphicshader.hpp"


ShaderGraphics::ShaderGraphics()
{
}

ShaderGraphics::~ShaderGraphics()
{
	for (auto& pipelines : m_pipelineStateObjects)
	{
		for (auto& psoPair : pipelines.second)
		{
			vkDestroyPipelineLayout(VkGlobals::vkDevice, psoPair.second.vkPipelineLayout, VkGlobals::vkAllocatorCallback);
			vkDestroyPipeline(VkGlobals::vkDevice, psoPair.second.vkPipeline, VkGlobals::vkAllocatorCallback);
		}
	}
}

void ShaderGraphics::Bind(VkCommandBuffer commandBuffer)
{
	std::vector<VkDescriptorSet> descriptorSets = { Shader::PerFrameDescriptors::vkDesciptorSet };
	if (m_descriptorSetCache != nullptr)
	{
		descriptorSets.push_back(*m_descriptorSetCache);
	}

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_psoCache->vkPipelineLayout,
		0,
		uint32_t(descriptorSets.size()),
		descriptorSets.data(),
		0, nullptr
	);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_psoCache->vkPipeline);
}

void ShaderGraphics::SetState(const Renderpass& renderpass, uint64_t bitmask, uint32_t descriptorSetMask, const RenderState& renderState)
{
	VulkanShader& shader = CompileStages(bitmask);
	PipeStateObjMap& pipelineStateObjectsMap = m_pipelineStateObjects[bitmask];

	CreatePerDrawcallDescriptorSet(shader, descriptorSetMask);
	CreateGraphicsPso(shader, pipelineStateObjectsMap, renderpass, renderState);
}

void ShaderGraphics::CreateGraphicsPso(VulkanShader& shader, PipeStateObjMap& pipelineStateObjectsMap, const Renderpass& renderpass, const RenderState& renderState)
{
	const uint64_t rndhash = renderState.Hash();
	if (pipelineStateObjectsMap.find(rndhash) != pipelineStateObjectsMap.end())
	{
		m_psoCache = &pipelineStateObjectsMap[rndhash];
		return;
	}

	m_psoCache = &pipelineStateObjectsMap[rndhash];

	CreatePipelineLayout(shader, *m_psoCache);

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssembly.topology = to_vk_enum(renderState.topology);

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
	colorBlendAttachments.reserve(renderpass.GetColorCount());
	for (size_t i(0); i < renderpass.GetColorCount(); ++i)
	{
		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachments.push_back(colorBlendAttachment);
	}

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendStateCreateInfo.attachmentCount = uint32_t(colorBlendAttachments.size());
	colorBlendStateCreateInfo.pAttachments = colorBlendAttachments.data();
	colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

	VkPipelineRasterizationStateCreateInfo rasterizer = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = to_vk_enum(renderState.fillMode);
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = to_vk_enum(renderState.cullMode);
	rasterizer.frontFace = to_vk_enum(renderState.frontFace);
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencilInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthStencilInfo.depthTestEnable = VK_TRUE;
	depthStencilInfo.depthWriteEnable = VK_TRUE;
	depthStencilInfo.depthCompareOp = to_vk_enum(renderState.depthFunc);
	depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilInfo.minDepthBounds = 0.0f;
	depthStencilInfo.maxDepthBounds = 1.0f;
	depthStencilInfo.stencilTestEnable = VK_FALSE;



	VkVertexInputBindingDescription vertexBindingDescriptions = {};
	std::vector<VkVertexInputAttributeDescription> attributeDescription;
	if (renderState.hasInputAttachment)
	{
		vertexBindingDescriptions.binding = 0;
		vertexBindingDescriptions.stride = sizeof(Vertex);
		vertexBindingDescriptions.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		attributeDescription.resize(5);
		attributeDescription[0].binding = 0;
		attributeDescription[0].location = 0;
		attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription[0].offset = 0;

		attributeDescription[1].binding = 0;
		attributeDescription[1].location = 1;
		attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription[1].offset = 12;

		attributeDescription[2].binding = 0;
		attributeDescription[2].location = 2;
		attributeDescription[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription[2].offset = 24;

		attributeDescription[3].binding = 0;
		attributeDescription[3].location = 3;
		attributeDescription[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription[3].offset = 36;

		attributeDescription[4].binding = 0;
		attributeDescription[4].location = 4;
		attributeDescription[4].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescription[4].offset = 48;
	}

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBindingDescriptions;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = uint32_t(attributeDescription.size());
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescription.data();



	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicStateCreateInfo.dynamicStateCount = (uint32_t)dynamicStates.size();
	dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

	VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineCreateInfo.stageCount = static_cast<uint32_t>(shader.shaderStages.size());
	pipelineCreateInfo.pStages = shader.shaderStages.data();
	pipelineCreateInfo.renderPass = renderpass.Get();
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
	pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizer;
	pipelineCreateInfo.pDepthStencilState = &depthStencilInfo;
	pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.layout = m_psoCache->vkPipelineLayout;

	VK_ASSERT(vkCreateGraphicsPipelines(VkGlobals::vkDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, VkGlobals::vkAllocatorCallback, &m_psoCache->vkPipeline));
}

RenderState::RenderState()
	: cullMode(ECull::None)
	, topology(ETopology::Triangle)
	, fillMode(EFillMode::Fill)
	, frontFace(EFrontFace::CCW)
	, depthFunc(EDepthFunc::Less)
	, hasInputAttachment(true)
{
}

uint64_t RenderState::Hash() const
{
	uint64_t hash = 0;
	hash += uint64_t(cullMode) * 1;
	hash += uint64_t(topology) * 10;
	hash += uint64_t(fillMode) * 100;
	hash += uint64_t(frontFace) * 1000;
	hash += uint64_t(depthFunc) * 10000;
	hash += uint64_t(hasInputAttachment) * 100000;

	return hash;
}