#pragma once
#include "vkshader.hpp"


struct RenderState
{
	ECull cullMode;
	ETopology topology;
	EFillMode fillMode;
	EFrontFace frontFace;
	EDepthFunc depthFunc;
	bool hasInputAttachment : 1;
	bool depthWrite : 1;

	RenderState();
	uint64_t Hash() const;
};

class ShaderGraphics : public Shader
{
	using PipeStateObjMap = std::unordered_map<uint64_t, PipeStateObj>; // key - render state
public:
	ShaderGraphics();
	~ShaderGraphics();

	void Bind(VkCommandBuffer commandBuffer) override;

	void SetState(const Renderpass& renderpass, uint64_t bitmask, uint32_t descriptorSetMask, const RenderState& renderState);

private:
	void CreateGraphicsPso(VulkanShader& shader, PipeStateObj& pipelineStateObj, const Renderpass& renderpass, const RenderState& renderState);

private:
	PipeStateObj m_psoCache;
};