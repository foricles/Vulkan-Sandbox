#pragma once
#include "vkcommon.hpp"
#include "vkengine.hpp"
#include "vkutils.hpp"



class Framebuffer
{
	friend class Renderpass;
public:
	Framebuffer();
	Framebuffer(Framebuffer&& framebuffer) noexcept;
	~Framebuffer();

	inline operator VkFramebuffer& () { return m_vkFramebuffer; }
	Framebuffer& operator=(Framebuffer&& framebuffer) noexcept;

public:
	Framebuffer(const Framebuffer&) = delete;
	Framebuffer& operator=(const Framebuffer&) = delete;

private:
	VkFramebuffer m_vkFramebuffer;
};


class Renderpass
{
	friend class RenderpassBuilder;
public:
	Renderpass();
	Renderpass(Renderpass&& renderpass) noexcept;
	~Renderpass();

	inline operator VkRenderPass& () { return m_vkRenderPass; }
	inline const VkRenderPass& Get() const { return m_vkRenderPass; }
	Renderpass& operator=(Renderpass&& renderpass) noexcept;
	
	Framebuffer CreateFramebuffer(uint32_t width, uint32_t height, const std::vector<VkImageView>& views) const;
	uint32_t GetColorCount() const { return m_colorCount; }
	bool HasDepth() const { return m_hasDepth; }

public:
	Renderpass(const Renderpass&) = delete;
	Renderpass& operator=(const Renderpass&) = delete;

private:
	VkRenderPass m_vkRenderPass;
	uint32_t m_colorCount;
	bool m_hasDepth;
};


class RenderpassBuilder
{
public:
	Renderpass Create();

	RenderpassBuilder& AddAttachment(VkFormat format, VkImageLayout initialLayout, VkImageLayout finalLayout);
	RenderpassBuilder& IsDepth();
	RenderpassBuilder& ClearOnBegin();

public:
	RenderpassBuilder& AddAttachment(EPixelFormat format, VkImageLayout initialLayout, VkImageLayout finalLayout) { return AddAttachment(to_vk_enum(format), initialLayout, finalLayout); }

private:
	bool usedDepth{ false };
	uint32_t count{ 0 };
	std::vector<VkAttachmentDescription> m_AttachmentDescriptions;
	std::vector<VkAttachmentReference> m_AttachmentReferences;
};