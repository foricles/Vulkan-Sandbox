#pragma once
#include "vkshader.hpp"



class ShaderRaytrace : public Shader
{
public:
	struct PipeStateObjWithShaderBindTable
	{
		PipeStateObj pipelineStateObject;

		VkBuffer tableRaygen{ VK_NULL_HANDLE };
		VkDeviceMemory tableRaygenMemory{ VK_NULL_HANDLE };
		VkStridedDeviceAddressRegionKHR tableRaygenAddressRegion{};

		VkBuffer tableHit{ VK_NULL_HANDLE };
		VkDeviceMemory tableHitMemory{ VK_NULL_HANDLE };
		VkStridedDeviceAddressRegionKHR tableHitAddressRegion{};

		VkBuffer tableMiss{ VK_NULL_HANDLE };
		VkDeviceMemory tableMissMemory{ VK_NULL_HANDLE };
		VkStridedDeviceAddressRegionKHR tableMissAddressRegion{};
	};

public: 
	ShaderRaytrace();
	~ShaderRaytrace();

	void SetState(uint64_t bitmask, uint32_t descriptorSetMask);

	void Bind(VkCommandBuffer commandBuffer) override;

	void Draw(VkCommandBuffer commandBuffer, uint32_t width, uint32_t height);

private:
	void CreatePipeline(VulkanShader& shader, PipeStateObjWithShaderBindTable& pipelineAndTables, uint32_t descriptorSetMask);

private:
	uint64_t m_bitmask{ 0 };
	PipeStateObj m_psoCache;
};