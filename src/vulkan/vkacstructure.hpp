#pragma once
#include "vkengine.hpp"
#include "vkcommon.hpp"
#include "vkbuffer.hpp"

class AccelerationStructure;

class AccStructBuilder
{
public:
	AccStructBuilder(AccelerationStructure* pAccStructure, VkAccelerationStructureTypeKHR kind);
	~AccStructBuilder();

	AccStructBuilder& AddTriangles(const Buffer& vertices, const Buffer& indices);
	AccStructBuilder& Primitives(uint32_t primitives);
	AccStructBuilder& MaxVertices(uint32_t maxVertices);
	AccStructBuilder& Stride(uint32_t stride);

	AccStructBuilder AddAccelerationStructure(AccelerationStructure* blac);

	void Build();

private:
	AccelerationStructure* m_pAccStructure;
	VkAccelerationStructureTypeKHR m_kind;
	std::vector<uint32_t> m_primitivesCount;
	std::vector<VkAccelerationStructureGeometryKHR> m_geometries;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR> m_ranges;
	std::vector<VkBuffer> m_instanceBuffers;
	std::vector<VkDeviceMemory> m_instanceBuffersMemory;
};

class AccelerationStructure
{
	friend class AccStructBuilder;
public:
	AccelerationStructure();
	AccelerationStructure(AccelerationStructure&& acstructure) noexcept;
	~AccelerationStructure();


	AccelerationStructure& operator=(AccelerationStructure&& acstructure) noexcept;

	VkDeviceAddress GetAddress() const;
	VkDeviceAddress GetBufferAddress() const;

	AccStructBuilder Builder(VkAccelerationStructureTypeKHR kind) { return AccStructBuilder(this, kind); }

public:
	VkBuffer GetBuffer() const { return m_vkAcBuffer; }
	VkAccelerationStructureKHR Get() const { return m_vkAccelerationStructure; }

public:
	AccelerationStructure(const AccelerationStructure&) = delete;
	AccelerationStructure& operator=(const AccelerationStructure&) = delete;

private:
	VkBuffer m_vkAcBuffer;
	VkDeviceMemory m_vkAcBufferMemory;
	VkAccelerationStructureKHR m_vkAccelerationStructure;
};