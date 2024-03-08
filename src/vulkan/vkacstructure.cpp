#include "vkacstructure.hpp"
#include "vkutils.hpp"
#include <cassert>



AccelerationStructure::AccelerationStructure()
	: m_vkAcBuffer(VK_NULL_HANDLE)
	, m_vkAcBufferMemory(VK_NULL_HANDLE)
	, m_vkAccelerationStructure(VK_NULL_HANDLE)
{
}

AccelerationStructure::AccelerationStructure(AccelerationStructure&& acstructure) noexcept
	: m_vkAcBuffer(acstructure.m_vkAcBuffer)
	, m_vkAcBufferMemory(acstructure.m_vkAcBufferMemory)
	, m_vkAccelerationStructure(acstructure.m_vkAccelerationStructure)
{
	acstructure.m_vkAcBuffer = VK_NULL_HANDLE;
	acstructure.m_vkAcBufferMemory = VK_NULL_HANDLE;
	acstructure.m_vkAccelerationStructure = VK_NULL_HANDLE;
}

AccelerationStructure& AccelerationStructure::operator = (AccelerationStructure && acstructure) noexcept
{
	std::swap(m_vkAcBuffer, acstructure.m_vkAcBuffer);
	std::swap(m_vkAcBufferMemory, acstructure.m_vkAcBufferMemory);
	std::swap(m_vkAccelerationStructure, acstructure.m_vkAccelerationStructure);
	return *this;
}

AccelerationStructure::~AccelerationStructure()
{
	static PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructure = (PFN_vkDestroyAccelerationStructureKHR)vkGetInstanceProcAddr(VkGlobals::vkInstance, "vkDestroyAccelerationStructureKHR");
	assert(vkDestroyAccelerationStructure != nullptr);

	if (m_vkAccelerationStructure != VK_NULL_HANDLE)
	{
		vkDestroyAccelerationStructure(VkGlobals::vkDevice, m_vkAccelerationStructure, VkGlobals::vkAllocatorCallback);
		m_vkAccelerationStructure = VK_NULL_HANDLE;
	}
	if (m_vkAcBufferMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(VkGlobals::vkDevice, m_vkAcBufferMemory, VkGlobals::vkAllocatorCallback);
		m_vkAcBufferMemory = VK_NULL_HANDLE;
	}
	if (m_vkAcBuffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(VkGlobals::vkDevice, m_vkAcBuffer, VkGlobals::vkAllocatorCallback);
		m_vkAcBuffer = VK_NULL_HANDLE;
	}
}





VkDeviceAddress AccelerationStructure::GetAddress() const
{
	static PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddress = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetInstanceProcAddr(VkGlobals::vkInstance, "vkGetAccelerationStructureDeviceAddressKHR");
	assert(vkGetAccelerationStructureDeviceAddress != nullptr);

	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
	accelerationDeviceAddressInfo.accelerationStructure = m_vkAccelerationStructure;
	return vkGetAccelerationStructureDeviceAddress(VkGlobals::vkDevice, &accelerationDeviceAddressInfo);
}

VkDeviceAddress AccelerationStructure::GetBufferAddress() const
{
	VkBufferDeviceAddressInfo pInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
	pInfo.buffer = m_vkAcBuffer;
	return vkGetBufferDeviceAddress(VkGlobals::vkDevice, &pInfo);
}





AccStructBuilder::AccStructBuilder(AccelerationStructure* pAccStructure, VkAccelerationStructureTypeKHR kind)
	: m_pAccStructure(pAccStructure)
	, m_kind(kind)
	, m_primitivesCount()
	, m_geometries()
	, m_ranges()
	, m_instanceBuffers()
	, m_instanceBuffersMemory()
{
}

AccStructBuilder::~AccStructBuilder()
{
}


AccStructBuilder& AccStructBuilder::AddTriangles(const Buffer& vertices, const Buffer& indices)
{
	VkAccelerationStructureGeometryKHR modelGeometry = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
	modelGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	modelGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	modelGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	modelGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	modelGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	modelGeometry.geometry.triangles.indexData.deviceAddress = indices.GetDeviceAddress();
	modelGeometry.geometry.triangles.vertexData.deviceAddress = vertices.GetDeviceAddress();
	modelGeometry.geometry.triangles.maxVertex = 0;
	modelGeometry.geometry.triangles.vertexStride = 0;
	//VkDeviceOrHostAddressConstKHR    transformData; Indicate identity transform by setting transformData to null device pointer.

	VkAccelerationStructureBuildRangeInfoKHR modelOffset = {};
	modelOffset.primitiveCount = 0;
	modelOffset.primitiveOffset = 0;
	modelOffset.firstVertex = 0;
	modelOffset.transformOffset = 0;

	m_geometries.push_back(modelGeometry);
	m_ranges.push_back(modelOffset);

	return *this;
}

AccStructBuilder& AccStructBuilder::Primitives(uint32_t primitives)
{
	m_primitivesCount.push_back(primitives);
	m_ranges.back().primitiveCount = primitives;
	return *this;
}

AccStructBuilder& AccStructBuilder::MaxVertices(uint32_t maxVertices)
{
	m_geometries.back().geometry.triangles.maxVertex = maxVertices - 1;
	return *this;
}

AccStructBuilder& AccStructBuilder::Stride(uint32_t stride)
{
	m_geometries.back().geometry.triangles.vertexStride = stride;
	return *this;
}


AccStructBuilder AccStructBuilder::AddAccelerationStructure(AccelerationStructure* blac)
{
	VkTransformMatrixKHR transformMatrix = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
	};

	VkAccelerationStructureInstanceKHR acInstance = {};
	acInstance.transform = transformMatrix;
	acInstance.instanceCustomIndex = 0;
	acInstance.mask = 0xFF;
	acInstance.instanceShaderBindingTableRecordOffset = 0;
	acInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
	acInstance.accelerationStructureReference = blac->GetAddress();

	VkBuffer accelerationInstanceBuffer = VK_NULL_HANDLE;
	VkBufferCreateInfo accelerationInstanceBufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	accelerationInstanceBufferCreateInfo.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	accelerationInstanceBufferCreateInfo.size = sizeof(VkAccelerationStructureInstanceKHR);
	VK_ASSERT(vkCreateBuffer(VkGlobals::vkDevice, &accelerationInstanceBufferCreateInfo, VkGlobals::vkAllocatorCallback, &accelerationInstanceBuffer));

	VkDeviceMemory accelerationInstanceBufferMemoty = VulkanEngine::AllocateMemory(accelerationInstanceBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR);

	VK_ASSERT(vkBindBufferMemory(VkGlobals::vkDevice, accelerationInstanceBuffer, accelerationInstanceBufferMemoty, 0));
	void* pMapped = nullptr;
	vkMapMemory(VkGlobals::vkDevice, accelerationInstanceBufferMemoty, VkDeviceSize(0), sizeof(VkAccelerationStructureInstanceKHR), 0, &pMapped);
	memcpy(pMapped, &acInstance, sizeof(VkAccelerationStructureInstanceKHR));


	VkBufferDeviceAddressInfoKHR accelerationInstanceBufferAddresInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
	accelerationInstanceBufferAddresInfo.buffer = accelerationInstanceBuffer;
	VkDeviceAddress accelerationInstanceBufferAddress = vkGetBufferDeviceAddress(VkGlobals::vkDevice, &accelerationInstanceBufferAddresInfo);


	VkAccelerationStructureGeometryKHR instanceGeometry = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
	instanceGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	instanceGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	instanceGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	instanceGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
	instanceGeometry.geometry.instances.data.deviceAddress = accelerationInstanceBufferAddress;


	VkAccelerationStructureBuildRangeInfoKHR toplevelRange = {};
	toplevelRange.primitiveCount = 1;
	toplevelRange.primitiveOffset = 0;
	toplevelRange.firstVertex = 0;
	toplevelRange.transformOffset = 0;

	m_ranges.push_back(toplevelRange);
	m_primitivesCount.push_back(1);
	m_geometries.push_back(instanceGeometry);
	m_instanceBuffers.push_back(accelerationInstanceBuffer);
	m_instanceBuffersMemory.push_back(accelerationInstanceBufferMemoty);

	return *this;
}


void AccStructBuilder::Build()
{
	static PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizes = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetInstanceProcAddr(VkGlobals::vkInstance, "vkGetAccelerationStructureBuildSizesKHR");
	static PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructure = (PFN_vkCreateAccelerationStructureKHR)vkGetInstanceProcAddr(VkGlobals::vkInstance, "vkCreateAccelerationStructureKHR");
	static PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructures = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetInstanceProcAddr(VkGlobals::vkInstance, "vkCmdBuildAccelerationStructuresKHR");

	assert(vkCmdBuildAccelerationStructures != nullptr);
	assert(vkGetAccelerationStructureBuildSizes != nullptr);
	assert(vkCreateAccelerationStructure != nullptr);

	VkAccelerationStructureBuildGeometryInfoKHR buildInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
	buildInfo.type = m_kind;
	buildInfo.geometryCount = uint32_t(m_geometries.size());
	buildInfo.pGeometries = m_geometries.data();
	buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;

	// get buffer size for accelerationStructure
	VkAccelerationStructureBuildSizesInfoKHR accelerationBufferSize = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
	vkGetAccelerationStructureBuildSizes(
		VkGlobals::vkDevice,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&buildInfo,
		m_primitivesCount.data(),
		&accelerationBufferSize
	);

	// create buffer for accelerationStructure
	VkBufferCreateInfo accelerationBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	accelerationBufferInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	accelerationBufferInfo.size = accelerationBufferSize.accelerationStructureSize;
	vkCreateBuffer(VkGlobals::vkDevice, &accelerationBufferInfo, VkGlobals::vkAllocatorCallback, &m_pAccStructure->m_vkAcBuffer);

	m_pAccStructure->m_vkAcBufferMemory = VulkanEngine::AllocateMemory(m_pAccStructure->m_vkAcBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR);

	vkBindBufferMemory(VkGlobals::vkDevice, m_pAccStructure->m_vkAcBuffer, m_pAccStructure->m_vkAcBufferMemory, 0);

	// create acceleration structure
	VkAccelerationStructureCreateInfoKHR structureInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
	structureInfo.type = m_kind;
	structureInfo.buffer = m_pAccStructure->m_vkAcBuffer;
	structureInfo.size = accelerationBufferSize.accelerationStructureSize;

	vkCreateAccelerationStructure(VkGlobals::vkDevice, &structureInfo, VkGlobals::vkAllocatorCallback, &m_pAccStructure->m_vkAccelerationStructure);



	VkBuffer stagingAccelerationBuffer = VK_NULL_HANDLE;
	VkDeviceMemory stagingAccelerationBufferMemory = VK_NULL_HANDLE;

	// create staging buffer for building accelerationStructure
	VkBufferCreateInfo stagingAccelerationBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	stagingAccelerationBufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	stagingAccelerationBufferInfo.size = accelerationBufferSize.accelerationStructureSize;
	vkCreateBuffer(VkGlobals::vkDevice, &stagingAccelerationBufferInfo, VkGlobals::vkAllocatorCallback, &stagingAccelerationBuffer);

	stagingAccelerationBufferMemory = VulkanEngine::AllocateMemory(stagingAccelerationBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR);

	vkBindBufferMemory(VkGlobals::vkDevice, stagingAccelerationBuffer, stagingAccelerationBufferMemory, 0);

	VkBufferDeviceAddressInfoKHR bufferDeviceAddresInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
	bufferDeviceAddresInfo.buffer = stagingAccelerationBuffer;
	VkDeviceAddress stagingAccelerationBufferAddress = vkGetBufferDeviceAddress(VkGlobals::vkDevice, &bufferDeviceAddresInfo);


	buildInfo.dstAccelerationStructure = m_pAccStructure->m_vkAccelerationStructure;
	buildInfo.scratchData.deviceAddress = stagingAccelerationBufferAddress;

	VulkanEngine::SubmitOnce([&](VkCommandBuffer commandBuffer) {
		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> copyInfos(m_ranges.size());
		for (uint32_t i(0); i < m_ranges.size(); ++i) {
			copyInfos[i] = &m_ranges[i];
		}
		vkCmdBuildAccelerationStructures(commandBuffer, 1, &buildInfo, copyInfos.data());
	});

	vkFreeMemory(VkGlobals::vkDevice, stagingAccelerationBufferMemory, VkGlobals::vkAllocatorCallback);
	vkDestroyBuffer(VkGlobals::vkDevice, stagingAccelerationBuffer, VkGlobals::vkAllocatorCallback);

	for (VkDeviceMemory& bufferMemory : m_instanceBuffersMemory)
	{
		vkFreeMemory(VkGlobals::vkDevice, bufferMemory, VkGlobals::vkAllocatorCallback);
	}

	for (VkBuffer& buffer : m_instanceBuffers)
	{
		vkDestroyBuffer(VkGlobals::vkDevice, buffer, VkGlobals::vkAllocatorCallback);
	}
}
