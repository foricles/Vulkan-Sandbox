#include "vkshader.hpp"
#include "vkengine.hpp"
#include "vkutils.hpp"
#include <Windows.h>
#include <dxc/dxcapi.h>
#include <wrl/client.h>
#include <codecvt>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include "spirv_reflect.h"



uint32_t				Shader::PerFrameDescriptors::counter = 0;
VkDescriptorSet			Shader::PerFrameDescriptors::vkDesciptorSet = VK_NULL_HANDLE;
VkDescriptorSetLayout	Shader::PerFrameDescriptors::vkDesciptorSetLayout = VK_NULL_HANDLE;


Shader::Shader()
	: Shader("")
{
}

Shader::Shader(std::string_view source)
	: m_psoCache(nullptr)
	, m_descriptorSetCache(VK_NULL_HANDLE)
	, m_stages()
	, m_source(source)
	, m_ShaderVariant()
{
	if (PerFrameDescriptors::counter <= 0)
	{
		VkDescriptorSetLayoutBinding perFrameBinding = {};
		perFrameBinding.binding = BUFFER_DS_OFFSET + 0;
		perFrameBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		perFrameBinding.descriptorCount = 1;
		perFrameBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutCreateInfo perFrameLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		perFrameLayoutInfo.bindingCount = 1;
		perFrameLayoutInfo.pBindings = &perFrameBinding;
		VK_ASSERT(vkCreateDescriptorSetLayout(VkGlobals::vkDevice, &perFrameLayoutInfo, VkGlobals::vkAllocatorCallback, &PerFrameDescriptors::vkDesciptorSetLayout));

		VkDescriptorSetAllocateInfo perFrameAllocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		perFrameAllocateInfo.descriptorPool = VkGlobals::vkDescriptorPool;
		perFrameAllocateInfo.descriptorSetCount = 1;
		perFrameAllocateInfo.pSetLayouts = &PerFrameDescriptors::vkDesciptorSetLayout;
		VK_ASSERT(vkAllocateDescriptorSets(VkGlobals::vkDevice, &perFrameAllocateInfo, &PerFrameDescriptors::vkDesciptorSet));
	}
	++PerFrameDescriptors::counter;
}

Shader::~Shader()
{
	for (auto& shaderVariant : m_ShaderVariant)
	{
		for (auto& shaderStage : shaderVariant.second.shaderStages)
		{
			vkDestroyShaderModule(VkGlobals::vkDevice, shaderStage.module, VkGlobals::vkAllocatorCallback);
		}

		for (auto& descriptorSet : shaderVariant.second.m_PerDrawcallDescriptorSets)
		{
			vkFreeDescriptorSets(VkGlobals::vkDevice, VkGlobals::vkDescriptorPool, 1, &descriptorSet.second);
		}

		vkDestroyDescriptorSetLayout(VkGlobals::vkDevice, shaderVariant.second.vkPerDrawcallDesciptorSetLayout, VkGlobals::vkAllocatorCallback);

		for (auto& psoPair : shaderVariant.second.m_PSOs)
		{
			vkDestroyPipelineLayout(VkGlobals::vkDevice, psoPair.second.vkPipelineLayout, VkGlobals::vkAllocatorCallback);
			vkDestroyPipeline(VkGlobals::vkDevice, psoPair.second.vkPipeline, VkGlobals::vkAllocatorCallback);
		}
	}

	--PerFrameDescriptors::counter;
	if (PerFrameDescriptors::counter <= 0)
	{
		vkFreeDescriptorSets(VkGlobals::vkDevice, VkGlobals::vkDescriptorPool, 1, &PerFrameDescriptors::vkDesciptorSet);
		vkDestroyDescriptorSetLayout(VkGlobals::vkDevice, PerFrameDescriptors::vkDesciptorSetLayout, VkGlobals::vkAllocatorCallback);
	}
}

void Shader::SetSource(std::string_view source)
{
	m_source = source.data();
}

void Shader::MarkProgram(EShaderType eType, std::string_view entry)
{
	m_stages[(uint32_t)eType] = entry;
}

Shader::PipeStateObj& Shader::GetPipeStateObj()
{
	return *m_psoCache;
}

VkDescriptorSet& Shader::GetPerDrawCallSet()
{
	return *m_descriptorSetCache;
}

void Shader::SetComputeState(uint64_t bitmask, uint32_t descriptorSetMask)
{
	//RenderState computeState;
	//computeState.isCompute = true;
	//SetState(VK_NULL_HANDLE, bitmask, descriptorSetMask, computeState);
}

void Shader::SetState(const Renderpass& renderpass, uint64_t bitmask, uint32_t descriptorSetMask, const RenderState& renderState)
{
	if (m_ShaderVariant.find(bitmask) == m_ShaderVariant.end())
	{
		CompileStages(bitmask);
	}


	BitmaskFilter& shaderVariant = m_ShaderVariant[bitmask];

	const uint64_t rndhash = renderState.Hash();
	if (shaderVariant.m_PSOs.find(rndhash) != shaderVariant.m_PSOs.end())
	{
		m_psoCache = &shaderVariant.m_PSOs[rndhash];
	}
	else
	{
		m_psoCache = &CreatePso(bitmask, renderpass, renderState);
	}

	m_descriptorSetCache = nullptr;
	if (shaderVariant.vkPerDrawcallDesciptorSetLayout != VK_NULL_HANDLE)
	{
		if (shaderVariant.m_PerDrawcallDescriptorSets.find(descriptorSetMask) != shaderVariant.m_PerDrawcallDescriptorSets.end())
		{
			m_descriptorSetCache = &shaderVariant.m_PerDrawcallDescriptorSets[descriptorSetMask];
		}
		else
		{
			m_descriptorSetCache = &CreatePerDrawcallDescriptorSet(bitmask, descriptorSetMask);
		}
	}
}

void Shader::WriteSampler(const Sampler& sampler, uint32_t location)
{
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.sampler = sampler.Get();

	VkWriteDescriptorSet descriptorWrites = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptorWrites.dstSet = GetPerDrawCallSet();
	descriptorWrites.dstBinding = SAMPLER_DS_OFFSET + location;
	descriptorWrites.descriptorCount = 1;
	descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	descriptorWrites.pImageInfo = &imageInfo;
	descriptorWrites.dstArrayElement = 0;

	vkUpdateDescriptorSets(VkGlobals::vkDevice, 1, &descriptorWrites, 0, nullptr);
}

void Shader::WriteImage(const Texture& texture, uint32_t location, VkImageLayout imageLayout)
{
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = imageLayout;
	imageInfo.imageView = texture.GetView();

	VkWriteDescriptorSet descriptorWrites = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptorWrites.dstSet = GetPerDrawCallSet();
	descriptorWrites.dstBinding = SRV_DS_OFFSET + location;
	descriptorWrites.descriptorCount = 1;
	descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	descriptorWrites.pImageInfo = &imageInfo;
	descriptorWrites.dstArrayElement = 0;

	vkUpdateDescriptorSets(VkGlobals::vkDevice, 1, &descriptorWrites, 0, nullptr);
}

void Shader::WriteStorageImage(const Texture& texture, uint32_t location, VkImageLayout imageLayout)
{
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = imageLayout;
	imageInfo.imageView = texture.GetView();

	VkWriteDescriptorSet descriptorWrites = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptorWrites.dstSet = GetPerDrawCallSet();
	descriptorWrites.dstBinding = UAV_DS_OFFSET + location;
	descriptorWrites.descriptorCount = 1;
	descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrites.pImageInfo = &imageInfo;
	descriptorWrites.dstArrayElement = 0;

	vkUpdateDescriptorSets(VkGlobals::vkDevice, 1, &descriptorWrites, 0, nullptr);
}

void Shader::WriteStorageBuffer(const Buffer& buffer, uint32_t location)
{
	VkDescriptorBufferInfo info = buffer.GetDscInfo();
	VkWriteDescriptorSet writeDescriptorSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	writeDescriptorSet.dstSet = GetPerDrawCallSet();
	writeDescriptorSet.dstBinding = UAV_DS_OFFSET + location;
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	writeDescriptorSet.pBufferInfo = &info;
	vkUpdateDescriptorSets(VkGlobals::vkDevice, 1, &writeDescriptorSet, 0, nullptr);
}

void Shader::WriteStorageBufferReadonly(const Buffer& buffer, uint32_t location)
{
	VkDescriptorBufferInfo info = buffer.GetDscInfo();
	VkWriteDescriptorSet writeDescriptorSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	writeDescriptorSet.dstSet = GetPerDrawCallSet();
	writeDescriptorSet.dstBinding = SRV_DS_OFFSET + location;
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	writeDescriptorSet.pBufferInfo = &info;
	vkUpdateDescriptorSets(VkGlobals::vkDevice, 1, &writeDescriptorSet, 0, nullptr);
}

void Shader::WritePerFrameBuffer(const Buffer& buffer, uint32_t location)
{
	VkDescriptorBufferInfo info = buffer.GetDscInfo();
	VkWriteDescriptorSet writeDescriptorSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	writeDescriptorSet.dstSet = Shader::PerFrameDescriptors::vkDesciptorSet;
	writeDescriptorSet.dstBinding = BUFFER_DS_OFFSET + location;
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSet.pBufferInfo = &info;
	vkUpdateDescriptorSets(VkGlobals::vkDevice, 1, &writeDescriptorSet, 0, nullptr);
}

Shader::PipeStateObj& Shader::CreatePso(uint64_t bitmask, const Renderpass& renderpass, const RenderState& renderState)
{
	const uint64_t renderStateHash = renderState.Hash();
	BitmaskFilter& shaderVariant = m_ShaderVariant[bitmask];
	PipeStateObj& pso = shaderVariant.m_PSOs[renderStateHash];

	std::vector<VkDescriptorSetLayout> setLayouts = { PerFrameDescriptors::vkDesciptorSetLayout };
	if (shaderVariant.vkPerDrawcallDesciptorSetLayout != VK_NULL_HANDLE)
	{
		setLayouts.push_back(shaderVariant.vkPerDrawcallDesciptorSetLayout);
	}

	VkPipelineLayoutCreateInfo pipelinel = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelinel.setLayoutCount = uint32_t(setLayouts.size());
	pipelinel.pSetLayouts = setLayouts.data();
	VK_ASSERT(vkCreatePipelineLayout(VkGlobals::vkDevice, &pipelinel, VkGlobals::vkAllocatorCallback, &pso.vkPipelineLayout));

	if (renderState.isCompute)
	{
		CreateComputePso(shaderVariant, pso);
	}
	else
	{
		CreateGraphicsPso(shaderVariant, pso, renderpass, renderState);
	}


	return pso;
}

void Shader::CreateGraphicsPso(BitmaskFilter& shaderVariant, PipeStateObj& pso, const Renderpass& renderpass, const RenderState& renderState)
{
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
	pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderVariant.shaderStages.size());
	pipelineCreateInfo.pStages = shaderVariant.shaderStages.data();
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
	pipelineCreateInfo.layout = pso.vkPipelineLayout;

	VK_ASSERT(vkCreateGraphicsPipelines(VkGlobals::vkDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, VkGlobals::vkAllocatorCallback, &pso.vkPipeline));
}

void Shader::CreateComputePso(BitmaskFilter& shaderVariant, PipeStateObj& pso)
{
	VkComputePipelineCreateInfo computeInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	computeInfo.stage = shaderVariant.shaderStages[0];
	computeInfo.layout = pso.vkPipelineLayout;
	VK_ASSERT(vkCreateComputePipelines(VkGlobals::vkDevice, VK_NULL_HANDLE, 1, &computeInfo, VkGlobals::vkAllocatorCallback, &pso.vkPipeline));
}

VkDescriptorSet& Shader::CreatePerDrawcallDescriptorSet(uint64_t bitmask, uint32_t descriptorSetMask)
{
	BitmaskFilter& shaderVariant = m_ShaderVariant[bitmask];
	VkDescriptorSet& descriptorSet = shaderVariant.m_PerDrawcallDescriptorSets[descriptorSetMask];

	VkDescriptorSetAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocateInfo.descriptorPool = VkGlobals::vkDescriptorPool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &shaderVariant.vkPerDrawcallDesciptorSetLayout;
	VK_ASSERT(vkAllocateDescriptorSets(VkGlobals::vkDevice, &allocateInfo, &descriptorSet));

	return descriptorSet;
}

void Shader::CompileStages(uint64_t bitmask)
{
	BitmaskFilter& shaderVariant = m_ShaderVariant[bitmask];

	std::vector<std::string> macroses;
	macroses.reserve(64);
	for (uint32_t i(0); i < sizeof(bitmask) * 8; ++i)
	{
		if ((bitmask & (uint64_t(1ull) << i)) > 0)
		{
			macroses.push_back("_PERMUTATION" + std::to_string(i) + "_");
		}
	}

	std::vector<VkDescriptorSetLayoutBinding> perDrawcallBindingings;
	perDrawcallBindingings.reserve(10);
	shaderVariant.shaderStages.reserve(m_stages.size());

	for (uint32_t i(0); i < m_stages.size(); ++i)
	{
		if (m_stages[i].size() <= 0)
		{
			continue;
		}

		const EShaderType shaderType = (EShaderType)i;
		auto shaderProgram = CompileShaderProgram(m_stages[i], shaderType, macroses);

		assert(shaderProgram.has_value());

		ReflectShaderProgram(shaderProgram.value());

		VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		createInfo.codeSize = shaderProgram.value().spirv.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderProgram.value().spirv.data());

		VkShaderModule shaderModule = VK_NULL_HANDLE;
		VK_ASSERT(vkCreateShaderModule(VkGlobals::vkDevice, &createInfo, VkGlobals::vkAllocatorCallback, &shaderModule));

		VkPipelineShaderStageCreateInfo shaderStageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		shaderStageInfo.module = shaderModule;
		shaderStageInfo.pName = m_stages[i].c_str();
		shaderStageInfo.stage = to_vk_enum(shaderType);
		shaderVariant.shaderStages.push_back(shaderStageInfo);

		for (const ShaderProgram::Descriptor& descriptor : shaderProgram.value().perDrawcallDescriptos)
		{
			const auto fnd = std::find_if(perDrawcallBindingings.begin(), perDrawcallBindingings.end(),
				[&descriptor](const VkDescriptorSetLayoutBinding& descBinding) {
					return descriptor.binding == descBinding.binding;
				}
			);

			if (fnd != perDrawcallBindingings.end())
			{
				fnd->stageFlags |= to_vk_enum(shaderType);
			}
			else
			{
				VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
				descriptorSetLayoutBinding.binding = descriptor.binding;
				descriptorSetLayoutBinding.descriptorType = to_vk_enum(descriptor.type);
				descriptorSetLayoutBinding.descriptorCount = 1;
				descriptorSetLayoutBinding.stageFlags = to_vk_enum(shaderType);

				perDrawcallBindingings.push_back(descriptorSetLayoutBinding);
			}
		}
	}

	if (perDrawcallBindingings.size() > 0)
	{
		VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layoutInfo.bindingCount = uint32_t(perDrawcallBindingings.size());
		layoutInfo.pBindings = perDrawcallBindingings.data();
		VK_ASSERT(vkCreateDescriptorSetLayout(VkGlobals::vkDevice, &layoutInfo, VkGlobals::vkAllocatorCallback, &shaderVariant.vkPerDrawcallDesciptorSetLayout));
	}
}


#include <filesystem>
#include <unordered_set>
std::optional<ShaderProgram> Shader::CompileShaderProgram(const std::string_view mainName, EShaderType eType, const std::vector<std::string> defines)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;

	ShaderProgram shaderInstance;

	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils = {};
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler = {};
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(dxcUtils.ReleaseAndGetAddressOf()));
	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));

	std::wstring rootPath = (std::filesystem::current_path() / "shaders").wstring();

	std::vector<LPCWSTR> args;
	args.reserve(50);
	args.push_back(L"-Zpc");
#ifdef _DEBUG
	args.push_back(L"-Zi");
	args.push_back(L"-Zss");
	args.push_back(L"-Od");
#endif
	args.push_back(L"-HV"); args.push_back(L"2021");
	args.push_back(L"-spirv");

	for (auto& dsSet : { L"0", L"1" })
	{
		args.push_back(L"-fvk-b-shift"); args.push_back(BUFFER_DS_OFFSET_str); args.push_back(dsSet);
		args.push_back(L"-fvk-s-shift"); args.push_back(SAMPLER_DS_OFFSET_str); args.push_back(dsSet);
		args.push_back(L"-fvk-t-shift"); args.push_back(SRV_DS_OFFSET_str); args.push_back(dsSet);
		args.push_back(L"-fvk-u-shift"); args.push_back(UAV_DS_OFFSET_str); args.push_back(dsSet);
	}

	args.push_back(L"-fspv-target-env=vulkan1.3");
#ifdef _DEBUG
	args.push_back(L"-fspv-debug=vulkan-with-source");
#endif

	std::wstring_convert<convert_typeX, wchar_t> converterX;
	const auto wmainName = converterX.from_bytes(mainName.data());

	args.push_back(L"-E"); args.push_back(wmainName.c_str());
	args.push_back(L"-T");
	switch (eType)
	{
	case EShaderType::Vertex: args.push_back(L"vs_6_6"); break;
	case EShaderType::Fragment: args.push_back(L"ps_6_6"); break;
	case EShaderType::Compute: args.push_back(L"cs_6_6"); break;
	default:
		//TODO: insert log
		break;
	}

	std::vector<std::wstring> macros;
	if (defines.size() > 0)
	{
		macros.reserve(defines.size());
		for (auto& macro : defines)
		{
			std::wstring_convert<convert_typeX, wchar_t> converterX;
			macros.push_back(converterX.from_bytes(macro));
			args.push_back(L"-D");
			args.push_back(macros.back().c_str());
		}
	}

	args.push_back(L"-I"); args.push_back(rootPath.c_str());

	DxcBuffer dxcBuffer = { 0 };
	dxcBuffer.Ptr = m_source.c_str();
	dxcBuffer.Size = m_source.size();
	dxcBuffer.Encoding = 0;


	//TODO: piece of shit. refactor this
	class : public IDxcIncludeHandler
	{
	public:
		HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override
		{
			Microsoft::WRL::ComPtr<IDxcBlobEncoding> pEncoding;
			std::filesystem::path path = std::filesystem::path(pFilename).make_preferred();
			if (IncludedFiles.find(path.string()) != IncludedFiles.end())
			{
				// Return empty string blob if this file has been included before
				static const char nullStr[] = " ";
				dxcUtils->CreateBlobFromPinned(nullStr, ARRAYSIZE(nullStr), DXC_CP_ACP, pEncoding.GetAddressOf());
				*ppIncludeSource = pEncoding.Detach();
				return S_OK;
			}

			HRESULT hr = dxcUtils->LoadFile(path.wstring().c_str(), nullptr, pEncoding.GetAddressOf());
			if (SUCCEEDED(hr))
			{
				IncludedFiles.insert(path.string());
				*ppIncludeSource = pEncoding.Detach();
			}
			return hr;
		}

		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override { return E_NOINTERFACE; }
		ULONG STDMETHODCALLTYPE AddRef(void) override { return 0; }
		ULONG STDMETHODCALLTYPE Release(void) override { return 0; }

		std::unordered_set<std::string> IncludedFiles;
		Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils;
	} includeHandler;
	includeHandler.dxcUtils = dxcUtils;

	Microsoft::WRL::ComPtr<IDxcResult> dxcResult;
	dxcCompiler->Compile(&dxcBuffer, args.data(), (UINT32)args.size(), &includeHandler, IID_PPV_ARGS(&dxcResult));

	Microsoft::WRL::ComPtr<IDxcBlobUtf8> pErrors;
	dxcResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.GetAddressOf()), nullptr);
	if (pErrors && pErrors->GetStringLength() > 0)
	{
		std::cout << "Shader compiling error: " << (char*)pErrors->GetStringPointer() << std::endl;
		return std::nullopt;
	}
	else
	{
		Microsoft::WRL::ComPtr<IDxcBlob> shaderObj;
		dxcResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderObj), nullptr);

		shaderInstance.spirv.resize(shaderObj->GetBufferSize());
		::memcpy(shaderInstance.spirv.data(), shaderObj->GetBufferPointer(), shaderObj->GetBufferSize());
	}

	return shaderInstance;
}

void Shader::ReflectShaderProgram(ShaderProgram& shaderProgram)
{
	SpvReflectShaderModule rmodule;
	SpvReflectResult result = spvReflectCreateShaderModule(shaderProgram.spirv.size(), shaderProgram.spirv.data(), &rmodule);

	uint32_t desCount = 0;
	std::vector<SpvReflectDescriptorSet*> descriptorSets;
	spvReflectEnumerateDescriptorSets(&rmodule, &desCount, nullptr);
	descriptorSets.resize(desCount);
	spvReflectEnumerateDescriptorSets(&rmodule, &desCount, descriptorSets.data());

	for (SpvReflectDescriptorSet* descriptorSet : descriptorSets)
	{
		for (uint32_t i(0); i < descriptorSet->binding_count; ++i)
		{
			ShaderProgram::Descriptor descriptor = {};

			SpvReflectDescriptorBinding* descriptorBinding = descriptorSet->bindings[i];
			descriptor.binding = descriptorBinding->binding;
			descriptor.size = descriptorBinding->block.size;

			switch (descriptorBinding->descriptor_type)
			{
			case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
			{
				descriptor.type = EDescriptorType::Sampler;
				break;
			}
			case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			{
				descriptor.type = EDescriptorType::SampledImage;
				break;
			}
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			{
				descriptor.type = EDescriptorType::StorageImage;
				break;
			}
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			{
				descriptor.type = EDescriptorType::UniformBuffer;
				break;
			}
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			{
				descriptor.type = EDescriptorType::StorageBuffer;
				break;
			}
			case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
			{
				descriptor.type = EDescriptorType::InputAttachment;
				break;
			}
			case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
			{
				descriptor.type = EDescriptorType::AccelerationStructure;
				break;
			}

			default:
				break;
			}

			if (descriptorBinding->set == 1)
			{
				shaderProgram.perDrawcallDescriptos.push_back(descriptor);
			}
			else if (descriptorBinding->set > 1)
			{
				//TOOD: error log
			}
		}
	}
}





RenderState::RenderState()
	: cullMode(ECull::None)
	, topology(ETopology::Triangle)
	, fillMode(EFillMode::Fill)
	, frontFace(EFrontFace::CCW)
	, depthFunc(EDepthFunc::Less)
	, hasInputAttachment(true)
	, isCompute(false)
{
}

uint64_t RenderState::Hash() const
{
	if (isCompute)
	{
		return UINT64_MAX;
	}

	uint64_t hash = 0;
	hash += uint64_t(cullMode) * 1;
	hash += uint64_t(topology) * 10;
	hash += uint64_t(fillMode) * 100;
	hash += uint64_t(frontFace) * 1000;
	hash += uint64_t(depthFunc) * 10000;
	hash += uint64_t(hasInputAttachment) * 100000;

	return hash;
}