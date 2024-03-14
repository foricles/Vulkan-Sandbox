#include "vkshader.hpp"
#include "../vkengine.hpp"
#include "../vkutils.hpp"
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
	: m_descriptorSetCache(VK_NULL_HANDLE)
	, m_stages()
	, m_source()
	, m_ShaderVariant()
{
	if (PerFrameDescriptors::counter <= 0)
	{
		VkDescriptorSetLayoutBinding perFrameBinding = {};
		perFrameBinding.binding = BUFFER_DS_OFFSET + 0;
		perFrameBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		perFrameBinding.descriptorCount = 1;
		perFrameBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
			| VK_SHADER_STAGE_GEOMETRY_BIT
			| VK_SHADER_STAGE_COMPUTE_BIT
			| VK_SHADER_STAGE_VERTEX_BIT
			| VK_SHADER_STAGE_RAYGEN_BIT_KHR;

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
	for (auto& shader : m_ShaderVariant)
	{
		for (auto& shaderStage : shader.second.shaderStages)
		{
			vkDestroyShaderModule(VkGlobals::vkDevice, shaderStage.module, VkGlobals::vkAllocatorCallback);
		}

		for (auto& descriptorSet : shader.second.perDrawcallDescriptorSets)
		{
			vkFreeDescriptorSets(VkGlobals::vkDevice, VkGlobals::vkDescriptorPool, 1, &descriptorSet.second);
		}

		vkDestroyDescriptorSetLayout(VkGlobals::vkDevice, shader.second.vkPerDrawcallDesciptorSetLayout, VkGlobals::vkAllocatorCallback);
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
	m_stages.push_back(std::make_pair(eType, std::string(entry)));
}

void Shader::CreatePipelineLayout(VulkanShader& shader, PipeStateObj& pipelineStateObject)
{
	std::vector<VkDescriptorSetLayout> setLayouts = { PerFrameDescriptors::vkDesciptorSetLayout };
	if (shader.vkPerDrawcallDesciptorSetLayout != VK_NULL_HANDLE)
	{
		setLayouts.push_back(shader.vkPerDrawcallDesciptorSetLayout);
	}

	VkPipelineLayoutCreateInfo pipelinel = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelinel.setLayoutCount = uint32_t(setLayouts.size());
	pipelinel.pSetLayouts = setLayouts.data();
	VK_ASSERT(vkCreatePipelineLayout(VkGlobals::vkDevice, &pipelinel, VkGlobals::vkAllocatorCallback, &pipelineStateObject.vkPipelineLayout));
}

void Shader::CreatePerDrawcallDescriptorSet(VulkanShader& shader, uint32_t descriptorSetMask)
{
	m_descriptorSetCache = VK_NULL_HANDLE;
	if (shader.vkPerDrawcallDesciptorSetLayout == VK_NULL_HANDLE)
	{
		return;
	}

	if (shader.perDrawcallDescriptorSets.find(descriptorSetMask) != shader.perDrawcallDescriptorSets.end())
	{
		m_descriptorSetCache = shader.perDrawcallDescriptorSets[descriptorSetMask];
		return;
	}

	VkDescriptorSetAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocateInfo.descriptorPool = VkGlobals::vkDescriptorPool;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &shader.vkPerDrawcallDesciptorSetLayout;
	VK_ASSERT(vkAllocateDescriptorSets(VkGlobals::vkDevice, &allocateInfo, &shader.perDrawcallDescriptorSets[descriptorSetMask]));

	m_descriptorSetCache = shader.perDrawcallDescriptorSets[descriptorSetMask];
}

Shader::VulkanShader& Shader::CompileStages(uint64_t bitmask)
{
	auto& fnd = m_ShaderVariant.find(bitmask);
	if (fnd != m_ShaderVariant.end())
	{
		return fnd->second;
	}

	VulkanShader& shader = m_ShaderVariant[bitmask];

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
	shader.shaderStages.reserve(m_stages.size());

	for (uint32_t i(0); i < m_stages.size(); ++i)
	{
		auto shaderProgram = CompileShaderProgram(m_stages[i].second, m_stages[i].first, macroses);

		assert(shaderProgram.has_value());

		ReflectShaderProgram(shaderProgram.value());

		VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		createInfo.codeSize = shaderProgram.value().spirv.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderProgram.value().spirv.data());

		VkShaderModule shaderModule = VK_NULL_HANDLE;
		VK_ASSERT(vkCreateShaderModule(VkGlobals::vkDevice, &createInfo, VkGlobals::vkAllocatorCallback, &shaderModule));

		VkPipelineShaderStageCreateInfo shaderStageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		shaderStageInfo.module = shaderModule;
		shaderStageInfo.pName = m_stages[i].second.c_str();
		shaderStageInfo.stage = to_vk_enum(m_stages[i].first);
		shader.shaderStages.push_back(shaderStageInfo);

		for (const ShaderProgram::Descriptor& descriptor : shaderProgram.value().perDrawcallDescriptos)
		{
			const auto fnd = std::find_if(perDrawcallBindingings.begin(), perDrawcallBindingings.end(),
				[&descriptor](const VkDescriptorSetLayoutBinding& descBinding) {
					return descriptor.binding == descBinding.binding;
				}
			);

			if (fnd != perDrawcallBindingings.end())
			{
				fnd->stageFlags |= to_vk_enum(m_stages[i].first);
			}
			else
			{
				VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
				descriptorSetLayoutBinding.binding = descriptor.binding;
				descriptorSetLayoutBinding.descriptorType = to_vk_enum(descriptor.type);
				descriptorSetLayoutBinding.descriptorCount = 1;
				descriptorSetLayoutBinding.stageFlags = to_vk_enum(m_stages[i].first);

				perDrawcallBindingings.push_back(descriptorSetLayoutBinding);
			}
		}
	}

	if (perDrawcallBindingings.size() > 0)
	{
		VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layoutInfo.bindingCount = uint32_t(perDrawcallBindingings.size());
		layoutInfo.pBindings = perDrawcallBindingings.data();
		VK_ASSERT(vkCreateDescriptorSetLayout(VkGlobals::vkDevice, &layoutInfo, VkGlobals::vkAllocatorCallback, &shader.vkPerDrawcallDesciptorSetLayout));
	}

	return shader;
}


#include <filesystem>
#include <unordered_set>
std::optional<Shader::ShaderProgram> Shader::CompileShaderProgram(const std::string_view mainName, EShaderType eType, const std::vector<std::string> defines)
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
	if (eType != EShaderType::RayGeneration
		&& eType != EShaderType::RayClosestHit
		&& eType != EShaderType::RayMiss)
	{
		args.push_back(L"-Zi");
		args.push_back(L"-Zss");
		args.push_back(L"-Od");
	}
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
	if (eType != EShaderType::RayGeneration
		&& eType != EShaderType::RayClosestHit
		&& eType != EShaderType::RayMiss)
	{
		args.push_back(L"-fspv-debug=vulkan-with-source");
	}
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
	case EShaderType::RayGeneration:
	case EShaderType::RayClosestHit:
	case EShaderType::RayMiss: args.push_back(L"lib_6_6"); break;
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



ShaderBinder::ShaderBinder(VkDescriptorSet descriptorSet)
	: descriptorSet(descriptorSet)
{
	images.reserve(16);
	buffers.reserve(16);
	writes.reserve(32);
	accelerationStructures.reserve(32);
	accelerationStructuresWrites.reserve(32);
}

ShaderBinder& ShaderBinder::ImageSampler(const Sampler& sampler, uint32_t location)
{
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.sampler = sampler.Get();
	images.push_back(imageInfo);

	VkWriteDescriptorSet descriptorWrites = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptorWrites.dstSet = descriptorSet;
	descriptorWrites.dstBinding = SAMPLER_DS_OFFSET + location;
	descriptorWrites.descriptorCount = 1;
	descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	descriptorWrites.pImageInfo = &images.back();
	descriptorWrites.dstArrayElement = 0;

	writes.push_back(descriptorWrites);

	return *this;
}

ShaderBinder& ShaderBinder::Image(const Texture& texture, uint32_t location, VkImageLayout imageLayout)
{
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = imageLayout;
	imageInfo.imageView = texture.GetView();
	images.push_back(imageInfo);

	VkWriteDescriptorSet descriptorWrites = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptorWrites.dstSet = descriptorSet;
	descriptorWrites.dstBinding = SRV_DS_OFFSET + location;
	descriptorWrites.descriptorCount = 1;
	descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	descriptorWrites.pImageInfo = &images.back();
	descriptorWrites.dstArrayElement = 0;

	writes.push_back(descriptorWrites);

	return *this;
}

ShaderBinder& ShaderBinder::StorageImage(const Texture& texture, uint32_t location, VkImageLayout imageLayout)
{
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = imageLayout;
	imageInfo.imageView = texture.GetView();
	images.push_back(imageInfo);

	VkWriteDescriptorSet descriptorWrites = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptorWrites.dstSet = descriptorSet;
	descriptorWrites.dstBinding = UAV_DS_OFFSET + location;
	descriptorWrites.descriptorCount = 1;
	descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrites.pImageInfo = &images.back();
	descriptorWrites.dstArrayElement = 0;

	writes.push_back(descriptorWrites);

	return *this;
}

ShaderBinder& ShaderBinder::UniformBuffer(const Buffer& buffer, uint32_t location)
{
	buffers.push_back(buffer.GetDscInfo());

	VkWriteDescriptorSet descriptorWrites = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptorWrites.dstSet = descriptorSet;
	descriptorWrites.dstBinding = BUFFER_DS_OFFSET + location;
	descriptorWrites.descriptorCount = 1;
	descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites.pBufferInfo = &buffers.back();

	writes.push_back(descriptorWrites);

	return *this;
}

ShaderBinder& ShaderBinder::StorageBuffer(const Buffer& buffer, uint32_t location)
{
	buffers.push_back(buffer.GetDscInfo());

	VkWriteDescriptorSet descriptorWrites = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptorWrites.dstSet = descriptorSet;
	descriptorWrites.dstBinding = UAV_DS_OFFSET + location;
	descriptorWrites.descriptorCount = 1;
	descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites.pBufferInfo = &buffers.back();

	writes.push_back(descriptorWrites);

	return *this;
}

ShaderBinder& ShaderBinder::StorageBufferReadonly(const Buffer& buffer, uint32_t location)
{
	buffers.push_back(buffer.GetDscInfo());

	VkWriteDescriptorSet descriptorWrites = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptorWrites.dstSet = descriptorSet;
	descriptorWrites.dstBinding = SRV_DS_OFFSET + location;
	descriptorWrites.descriptorCount = 1;
	descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites.pBufferInfo = &buffers.back();

	writes.push_back(descriptorWrites);

	return *this;
}

ShaderBinder& ShaderBinder::AccelerationStructure(const VkAccelerationStructureKHR& structure, uint32_t location)
{
	accelerationStructures.push_back(structure);

	VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR };
	descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
	descriptorAccelerationStructureInfo.pAccelerationStructures = &accelerationStructures.back();

	accelerationStructuresWrites.push_back(descriptorAccelerationStructureInfo);

	VkWriteDescriptorSet descriptorWrites = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptorWrites.dstSet = descriptorSet;
	descriptorWrites.dstBinding = SRV_DS_OFFSET + location;
	descriptorWrites.descriptorCount = 1;
	descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	descriptorWrites.pNext = &accelerationStructuresWrites.back();

	writes.push_back(descriptorWrites);

	return *this;
}

void ShaderBinder::Bind()
{
	vkUpdateDescriptorSets( VkGlobals::vkDevice
		, uint32_t(writes.size())
		, writes.data()
		, 0, nullptr
	);
}