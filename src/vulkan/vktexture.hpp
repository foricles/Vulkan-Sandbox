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

	inline operator VkImage& () { return m_vkImage; }
	Texture& operator=(Texture&& texture) noexcept;

	void Create(uint32_t width, uint32_t height, EPixelFormat format, uint32_t mip = 1);
	void Create(uint32_t width, uint32_t height, VkFormat format, uint32_t mip = 1);
	void Load(const std::vector<uint8_t>& data, uint32_t width, uint32_t height, EPixelFormat format, uint32_t mip = 1);
	bool IsDepth() const;
	bool IsDepthStencil() const;

public:
	inline uint32_t GetWidth() const { return m_width; }
	inline uint32_t GetHeight() const { return m_height; }
	inline VkImageView& GetView() { return m_vkImageView; }
	inline const VkImageView& GetView() const { return m_vkImageView; }

public:
	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;

private:
	void Free();

private:
	uint32_t m_width;
	uint32_t m_height;
	VkFormat m_format;
	VkImage m_vkImage;
	VkImageView m_vkImageView;
	VkDeviceMemory m_vkImageMemory;
};


class Sampler
{
public:
	Sampler();
	Sampler(Sampler&& sampler) noexcept;
	~Sampler();

	inline operator VkSampler& () { return m_vkSampler; }
	inline const VkSampler& Get() const { return m_vkSampler; }
	Sampler& operator=(Sampler&& sampler) noexcept;

	void Create(ESampleFilter filter, ESampleMode mode, float anisotropy, float mipCount);

public:
	Sampler(const Sampler&) = delete;
	Sampler& operator=(const Sampler&) = delete;

private:
	VkSampler m_vkSampler;
};