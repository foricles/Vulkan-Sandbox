#pragma once
#include "vkcommon.hpp"
#include "vkengine.hpp"
#include "vkutils.hpp"



class Texture
{
public:
	Texture();
	Texture(Texture&& texture) noexcept;
	~Texture();

	inline VkImage Get() const { return m_vkImage; }
	Texture& operator=(Texture&& texture) noexcept;

	void CreateCube(uint32_t width, uint32_t height, EPixelFormat format);
	void Create(uint32_t width, uint32_t height, EPixelFormat format, uint32_t mip = 1);
	void Create(uint32_t width, uint32_t height, VkFormat format, uint32_t mip = 1);
	void Load(const std::vector<uint8_t>& data, uint32_t width, uint32_t height, EPixelFormat format, uint32_t mip = 1);
	bool IsDepth() const;
	bool IsDepthStencil() const;

	void SetViewType(VkImageViewType type);
	void SetBarier(VkCommandBuffer commandBuffer,
		VkPipelineStageFlags2 srcStageMask,
		VkAccessFlags2 srcAccessMask,
		VkPipelineStageFlags2 dstStageMask,
		VkAccessFlags2 dstAccessMask,
		VkImageLayout newLayout);

public:
	inline uint32_t GetWidth() const { return m_width; }
	inline uint32_t GetHeight() const { return m_height; }
	inline const VkImageView GetView() const { return m_vkImageView; }

public:
	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;

private:
	void Free();

private:
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_mipLevels;
	uint32_t m_layerCount;
	VkFormat m_format;
	VkImage m_vkImage;
	VkImageView m_vkImageView;
	VkDeviceMemory m_vkImageMemory;
	VkImageLayout m_currentLayout;
	VkImageAspectFlags m_aspect;
};


class Sampler
{
public:
	Sampler();
	Sampler(Sampler&& sampler) noexcept;
	~Sampler();

	inline const VkSampler Get() const { return m_vkSampler; }
	Sampler& operator=(Sampler&& sampler) noexcept;

	void Create(ESampleFilter filter, ESampleMode mode, float anisotropy, float mipCount);

public:
	Sampler(const Sampler&) = delete;
	Sampler& operator=(const Sampler&) = delete;

private:
	VkSampler m_vkSampler;
};