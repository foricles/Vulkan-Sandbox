#include "raytraceshader.hpp"




ShaderRaytrace::ShaderRaytrace()
	: Shader()
{
}

ShaderRaytrace::~ShaderRaytrace()
{
	for (auto& shader : m_ShaderVariant)
	{
		auto& pipelineAndTables = shader.second.GetPso<PipeStateObjWithShaderBindTable>();
		vkDestroyPipeline(VkGlobals::vkDevice, pipelineAndTables.pipelineStateObject.vkPipeline, VkGlobals::vkAllocatorCallback);
		vkDestroyPipelineLayout(VkGlobals::vkDevice, pipelineAndTables.pipelineStateObject.vkPipelineLayout, VkGlobals::vkAllocatorCallback);
		vkFreeMemory(VkGlobals::vkDevice, pipelineAndTables.tableHitMemory, VkGlobals::vkAllocatorCallback);
		vkFreeMemory(VkGlobals::vkDevice, pipelineAndTables.tableMissMemory, VkGlobals::vkAllocatorCallback);
		vkFreeMemory(VkGlobals::vkDevice, pipelineAndTables.tableRaygenMemory, VkGlobals::vkAllocatorCallback);
		vkDestroyBuffer(VkGlobals::vkDevice, pipelineAndTables.tableHit, VkGlobals::vkAllocatorCallback);
		vkDestroyBuffer(VkGlobals::vkDevice, pipelineAndTables.tableMiss, VkGlobals::vkAllocatorCallback);
		vkDestroyBuffer(VkGlobals::vkDevice, pipelineAndTables.tableRaygen, VkGlobals::vkAllocatorCallback);
		pipelineAndTables.~PipeStateObjWithShaderBindTable();
	}
}

void ShaderRaytrace::Bind(VkCommandBuffer commandBuffer)
{
	std::vector<VkDescriptorSet> descriptorSets = { Shader::PerFrameDescriptors::vkDesciptorSet };
	if (m_descriptorSetCache != VK_NULL_HANDLE)
	{
		descriptorSets.push_back(m_descriptorSetCache);
	}

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
		m_psoCache.vkPipelineLayout,
		0,
		uint32_t(descriptorSets.size()),
		descriptorSets.data(),
		0, nullptr
	);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_psoCache.vkPipeline);
}

void ShaderRaytrace::SetState(uint64_t bitmask, uint32_t descriptorSetMask)
{
	m_bitmask = bitmask;
	VulkanShader& shader = CompileStages(bitmask);
	CreatePerDrawcallDescriptorSet(shader, descriptorSetMask);
	if (shader.IsPsoNull())
	{
		auto& pipelineAndTables = shader.NewPso<PipeStateObjWithShaderBindTable>();
		CreatePipelineLayout(shader, pipelineAndTables.pipelineStateObject);
		CreatePipeline(shader, pipelineAndTables, descriptorSetMask);
		m_psoCache = pipelineAndTables.pipelineStateObject;
	}
	else
	{
		auto& pipelineAndTables = shader.GetPso<PipeStateObjWithShaderBindTable>();
		m_psoCache = pipelineAndTables.pipelineStateObject;
	}
}

void ShaderRaytrace::CreatePipeline(VulkanShader& shader, PipeStateObjWithShaderBindTable& pipelineAndTables, uint32_t descriptorSetMask)
{
	static PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelines = (PFN_vkCreateRayTracingPipelinesKHR)vkGetInstanceProcAddr(VkGlobals::vkInstance, "vkCreateRayTracingPipelinesKHR");
	static PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandles = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetInstanceProcAddr(VkGlobals::vkInstance, "vkGetRayTracingShaderGroupHandlesKHR");
	assert(vkCreateRayTracingPipelines != nullptr);
	assert(vkGetRayTracingShaderGroupHandles != nullptr);


	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups(shader.shaderStages.size());
	for (uint32_t i(0); i < shader.shaderStages.size(); ++i)
	{
		shaderGroups[i].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroups[i].pNext = nullptr;
		shaderGroups[i].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		shaderGroups[i].generalShader = VK_SHADER_UNUSED_KHR;
		shaderGroups[i].closestHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroups[i].anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroups[i].intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups[i].pShaderGroupCaptureReplayHandle = nullptr;

		if (shader.shaderStages[i].stage == VK_SHADER_STAGE_RAYGEN_BIT_KHR)
		{
			shaderGroups[i].generalShader = i;
		}
		else if (shader.shaderStages[i].stage == VK_SHADER_STAGE_MISS_BIT_KHR)
		{
			shaderGroups[i].generalShader = i;
		}
		else if (shader.shaderStages[i].stage == VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
		{
			shaderGroups[i].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			shaderGroups[i].closestHitShader = i;
		}
		else
		{
			assert(false);
		}
	}


	VkRayTracingPipelineCreateInfoKHR raytracingPipelineInfo = { VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
    raytracingPipelineInfo.stageCount = uint32_t(shader.shaderStages.size());
    raytracingPipelineInfo.pStages = shader.shaderStages.data();
	raytracingPipelineInfo.groupCount = uint32_t(shaderGroups.size());
	raytracingPipelineInfo.pGroups = shaderGroups.data();
    raytracingPipelineInfo.maxPipelineRayRecursionDepth = 1;
	raytracingPipelineInfo.layout = pipelineAndTables.pipelineStateObject.vkPipelineLayout;

	vkCreateRayTracingPipelines(VkGlobals::vkDevice, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &raytracingPipelineInfo, VkGlobals::vkAllocatorCallback, &pipelineAndTables.pipelineStateObject.vkPipeline);

	




	VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracingProps = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };

	VkPhysicalDeviceProperties2 properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
	properties.pNext = &raytracingProps;
	vkGetPhysicalDeviceProperties2(VkGlobals::vkGPU, &properties);

	uint32_t handlerSize = raytracingProps.shaderGroupHandleSize;
	uint32_t handlerSizeAligned = (raytracingProps.shaderGroupHandleSize + raytracingProps.shaderGroupHandleAlignment - 1) & ~(raytracingProps.shaderGroupHandleAlignment - 1);
	uint32_t groupCount = uint32_t(shaderGroups.size());
	uint32_t shaderBindTableSize = groupCount * handlerSizeAligned;


	std::vector<uint8_t> shaderHandlerStorage(shaderBindTableSize);
	vkGetRayTracingShaderGroupHandles(VkGlobals::vkDevice, pipelineAndTables.pipelineStateObject.vkPipeline, 0, groupCount, shaderBindTableSize, shaderHandlerStorage.data());

	{
		VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		createInfo.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		createInfo.size = handlerSize;
		vkCreateBuffer(VkGlobals::vkDevice, &createInfo, VkGlobals::vkAllocatorCallback, &pipelineAndTables.tableRaygen);

		pipelineAndTables.tableRaygenMemory = VulkanEngine::AllocateMemory(pipelineAndTables.tableRaygen, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR);

		vkBindBufferMemory(VkGlobals::vkDevice, pipelineAndTables.tableRaygen, pipelineAndTables.tableRaygenMemory, 0);
		void* pMem = nullptr;
		vkMapMemory(VkGlobals::vkDevice, pipelineAndTables.tableRaygenMemory, 0, handlerSize, 0, &pMem);
		memcpy(pMem, shaderHandlerStorage.data() + handlerSizeAligned * 0, handlerSize);


		VkBufferDeviceAddressInfoKHR bufferDeviceAddresInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
		bufferDeviceAddresInfo.buffer = pipelineAndTables.tableRaygen;

		pipelineAndTables.tableRaygenAddressRegion.size = handlerSizeAligned;
		pipelineAndTables.tableRaygenAddressRegion.stride = handlerSizeAligned;
		pipelineAndTables.tableRaygenAddressRegion.deviceAddress = vkGetBufferDeviceAddress(VkGlobals::vkDevice, &bufferDeviceAddresInfo);
	}
	{
		VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		createInfo.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		createInfo.size = handlerSize;
		vkCreateBuffer(VkGlobals::vkDevice, &createInfo, VkGlobals::vkAllocatorCallback, &pipelineAndTables.tableHit);

		pipelineAndTables.tableHitMemory = VulkanEngine::AllocateMemory(pipelineAndTables.tableHit, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR);

		vkBindBufferMemory(VkGlobals::vkDevice, pipelineAndTables.tableHit, pipelineAndTables.tableHitMemory, 0);
		void* pMem = nullptr;
		vkMapMemory(VkGlobals::vkDevice, pipelineAndTables.tableHitMemory, 0, handlerSize, 0, &pMem);
		memcpy(pMem, shaderHandlerStorage.data() + handlerSizeAligned * 1, handlerSize);


		VkBufferDeviceAddressInfoKHR bufferDeviceAddresInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
		bufferDeviceAddresInfo.buffer = pipelineAndTables.tableHit;

		pipelineAndTables.tableHitAddressRegion.size = handlerSizeAligned;
		pipelineAndTables.tableHitAddressRegion.stride = handlerSizeAligned;
		pipelineAndTables.tableHitAddressRegion.deviceAddress = vkGetBufferDeviceAddress(VkGlobals::vkDevice, &bufferDeviceAddresInfo);
	}
	{
		VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		createInfo.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		createInfo.size = handlerSize;
		vkCreateBuffer(VkGlobals::vkDevice, &createInfo, VkGlobals::vkAllocatorCallback, &pipelineAndTables.tableMiss);

		pipelineAndTables.tableMissMemory = VulkanEngine::AllocateMemory(pipelineAndTables.tableMiss, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR);

		vkBindBufferMemory(VkGlobals::vkDevice, pipelineAndTables.tableMiss, pipelineAndTables.tableMissMemory, 0);
		void* pMem = nullptr;
		vkMapMemory(VkGlobals::vkDevice, pipelineAndTables.tableMissMemory, 0, handlerSize, 0, &pMem);
		memcpy(pMem, shaderHandlerStorage.data() + handlerSizeAligned * 2, handlerSize);


		VkBufferDeviceAddressInfoKHR bufferDeviceAddresInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
		bufferDeviceAddresInfo.buffer = pipelineAndTables.tableMiss;

		pipelineAndTables.tableMissAddressRegion.size = handlerSizeAligned;
		pipelineAndTables.tableMissAddressRegion.stride = handlerSizeAligned;
		pipelineAndTables.tableMissAddressRegion.deviceAddress = vkGetBufferDeviceAddress(VkGlobals::vkDevice, &bufferDeviceAddresInfo);
	}
}


void ShaderRaytrace::Draw(VkCommandBuffer commandBuffer, uint32_t width, uint32_t height)
{
	static PFN_vkCmdTraceRaysKHR vkCmdTraceRays = (PFN_vkCmdTraceRaysKHR)vkGetInstanceProcAddr(VkGlobals::vkInstance, "vkCmdTraceRaysKHR");
	assert(vkCmdTraceRays != nullptr);

	auto& pipelineAndTables = CompileStages(m_bitmask).GetPso<PipeStateObjWithShaderBindTable>();

	VkStridedDeviceAddressRegionKHR emptySbtEntry = {};
	vkCmdTraceRays(
		commandBuffer,
		&pipelineAndTables.tableRaygenAddressRegion,
		&pipelineAndTables.tableMissAddressRegion,
		&pipelineAndTables.tableHitAddressRegion,
		&emptySbtEntry,
		width, height, 1
	);
}