#include "vktexture.hpp"




Texture::Texture()
	: m_width(0)
	, m_height(0)
	, m_format(VK_FORMAT_R8_UNORM)
	, m_vkImage(VK_NULL_HANDLE)
	, m_vkImageView(VK_NULL_HANDLE)
    , m_vkImageMemory(VK_NULL_HANDLE)
{
}

Texture::~Texture()
{
    Free();
}

Texture::Texture(Texture&& texture) noexcept
{
}

Texture& Texture::operator=(Texture&& texture) noexcept
{
    return *this;
}

void Texture::Create(uint32_t width, uint32_t height, EPixelFormat format, uint32_t mip)
{
    Create(width, height, to_vk_enum(format), mip);
}

void Texture::Create(uint32_t width, uint32_t height, VkFormat format, uint32_t mip)
{
    m_width = width;
    m_height = height;
    m_format = format;

    if (m_vkImage != VK_NULL_HANDLE)
    {
        Free();
    }

    VkImageCreateInfo imageCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = mip;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (IsDepth() || IsDepthStencil())
    {
        imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    }

    VK_ASSERT(vkCreateImage(VkGlobals::vkDevice, &imageCreateInfo, VkGlobals::vkAllocatorCallback, &m_vkImage));


    m_vkImageMemory = VulkanEngine::AllocateMemory(m_vkImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_ASSERT(vkBindImageMemory(VkGlobals::vkDevice, m_vkImage, m_vkImageMemory, VkDeviceSize(0)));


    VkImageViewCreateInfo imageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    imageViewInfo.image = m_vkImage;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.format = format;
    imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.subresourceRange.levelCount = mip;
    imageViewInfo.subresourceRange.layerCount = 1;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    if (IsDepth())
    {
        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else if (IsDepthStencil())
    {
        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    VK_ASSERT(vkCreateImageView(VkGlobals::vkDevice, &imageViewInfo, VkGlobals::vkAllocatorCallback, &m_vkImageView));
}

void Texture::Load(const std::vector<uint8_t>& data, uint32_t width, uint32_t height, EPixelFormat format, uint32_t mip)
{
    Create(width, height, format, mip);

    VkBuffer vkStagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vkStagingMemory = VK_NULL_HANDLE;

    VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = uint32_t(data.size());
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(VkGlobals::vkDevice, &bufferInfo, VkGlobals::vkAllocatorCallback, &vkStagingBuffer);

    vkStagingMemory = VulkanEngine::AllocateMemory(vkStagingBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkBindBufferMemory(VkGlobals::vkDevice, vkStagingBuffer, vkStagingMemory, 0);

    void* pMem = nullptr;
    vkMapMemory(VkGlobals::vkDevice, vkStagingMemory, 0, uint32_t(data.size()), 0, &pMem);
    ::memcpy(pMem, data.data(), data.size());


    VulkanEngine::SubmitOnce([&](VkCommandBuffer commandBuffer) {
        // transfer image to dest optimal layout
        VkImageMemoryBarrier2 imageMemoryBarrierDestOptimal = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
        imageMemoryBarrierDestOptimal.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        imageMemoryBarrierDestOptimal.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        imageMemoryBarrierDestOptimal.srcAccessMask = 0;
        imageMemoryBarrierDestOptimal.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        imageMemoryBarrierDestOptimal.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrierDestOptimal.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrierDestOptimal.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrierDestOptimal.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrierDestOptimal.image = m_vkImage;
        imageMemoryBarrierDestOptimal.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageMemoryBarrierDestOptimal.subresourceRange.layerCount = 1;
        imageMemoryBarrierDestOptimal.subresourceRange.levelCount = mip;
        imageMemoryBarrierDestOptimal.subresourceRange.baseMipLevel = 0;
        imageMemoryBarrierDestOptimal.subresourceRange.baseArrayLayer = 0;

        VkDependencyInfo depinfoDestOptimal = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
        depinfoDestOptimal.imageMemoryBarrierCount = 1;
        depinfoDestOptimal.pImageMemoryBarriers = &imageMemoryBarrierDestOptimal;
        vkCmdPipelineBarrier2(commandBuffer, &depinfoDestOptimal);

        // copy data
        VkBufferImageCopy regions = {};
        regions.bufferOffset = 0;
        regions.bufferRowLength = 0;
        regions.bufferImageHeight = 0;
        regions.imageOffset = {0, 0, 0};
        regions.imageExtent = {m_width, m_height, 1};
        regions.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        regions.imageSubresource.layerCount = 1;
        regions.imageSubresource.mipLevel = 0;
        regions.imageSubresource.baseArrayLayer = 0;

        vkCmdCopyBufferToImage(commandBuffer, vkStagingBuffer, m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &regions);


        if (mip > 1)
        {
            uint32_t mip_width = m_width;
            uint32_t mip_height = m_height;

            VkImageMemoryBarrier2 imageMemoryBarrierMipGen = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
            imageMemoryBarrierMipGen.image = m_vkImage;
            imageMemoryBarrierMipGen.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrierMipGen.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrierMipGen.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            imageMemoryBarrierMipGen.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            imageMemoryBarrierMipGen.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageMemoryBarrierMipGen.subresourceRange.layerCount = 1;
            imageMemoryBarrierMipGen.subresourceRange.levelCount = 1;
            imageMemoryBarrierMipGen.subresourceRange.baseArrayLayer = 0;

            VkDependencyInfo depinfoMip = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
            depinfoMip.imageMemoryBarrierCount = 1;
            depinfoMip.pImageMemoryBarriers = &imageMemoryBarrierMipGen;
            for (uint32_t i(1); i < mip; ++i)
            {
                imageMemoryBarrierMipGen.subresourceRange.baseMipLevel = i - 1;
                imageMemoryBarrierMipGen.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                imageMemoryBarrierMipGen.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                imageMemoryBarrierMipGen.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                imageMemoryBarrierMipGen.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;

                vkCmdPipelineBarrier2(commandBuffer, &depinfoMip);

                VkImageBlit imageBlit = {};
                imageBlit.srcOffsets[0] = { 0,0,0 };
                imageBlit.srcOffsets[1] = { (int32_t)mip_width, (int32_t)mip_height, 1 };
                imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.srcSubresource.mipLevel = i - 1;
                imageBlit.srcSubresource.baseArrayLayer = 0;
                imageBlit.srcSubresource.layerCount = 1;

                mip_width = mip_width >> 1;
                mip_height = mip_height >> 1;

                imageBlit.dstOffsets[0] = { 0,0,0 };
                imageBlit.dstOffsets[1] = { (int32_t)mip_width, (int32_t)mip_width, 1 };
                imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.dstSubresource.mipLevel = i;
                imageBlit.dstSubresource.baseArrayLayer = 0;
                imageBlit.dstSubresource.layerCount = 1;

                vkCmdBlitImage(commandBuffer, m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);
            }

            imageMemoryBarrierMipGen.subresourceRange.baseMipLevel = mip - 1;
            imageMemoryBarrierMipGen.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrierMipGen.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrierMipGen.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrierMipGen.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            imageMemoryBarrierMipGen.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
            imageMemoryBarrierMipGen.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            vkCmdPipelineBarrier2(commandBuffer, &depinfoMip);
        }

        // transfer image to shader read layout
        VkImageMemoryBarrier2 imageMemoryBarrierShaderRead = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
        imageMemoryBarrierShaderRead.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        imageMemoryBarrierShaderRead.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        imageMemoryBarrierShaderRead.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        imageMemoryBarrierShaderRead.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
        imageMemoryBarrierShaderRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrierShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageMemoryBarrierShaderRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrierShaderRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrierShaderRead.image = m_vkImage;
        imageMemoryBarrierShaderRead.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageMemoryBarrierShaderRead.subresourceRange.layerCount = 1;
        imageMemoryBarrierShaderRead.subresourceRange.levelCount = mip;
        imageMemoryBarrierShaderRead.subresourceRange.baseMipLevel = 0;
        imageMemoryBarrierShaderRead.subresourceRange.baseArrayLayer = 0;
        if (mip > 1)
        {
            imageMemoryBarrierShaderRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrierShaderRead.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        }

        VkDependencyInfo depinfoShaderRead = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
        depinfoShaderRead.imageMemoryBarrierCount = 1;
        depinfoShaderRead.pImageMemoryBarriers = &imageMemoryBarrierShaderRead;
        vkCmdPipelineBarrier2(commandBuffer, &depinfoShaderRead);
     
    });

    vkFreeMemory(VkGlobals::vkDevice, vkStagingMemory, VkGlobals::vkAllocatorCallback);
    vkDestroyBuffer(VkGlobals::vkDevice, vkStagingBuffer, VkGlobals::vkAllocatorCallback);
    
}

bool Texture::IsDepth() const
{
    return m_format == VK_FORMAT_D32_SFLOAT;
}

bool Texture::IsDepthStencil() const
{
    return (m_format == VK_FORMAT_D24_UNORM_S8_UINT) || (m_format == VK_FORMAT_D32_SFLOAT_S8_UINT);
}

void Texture::Free()
{
    if (m_vkImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(VkGlobals::vkDevice, m_vkImageView, VkGlobals::vkAllocatorCallback);
        m_vkImageView = VK_NULL_HANDLE;
    }

    if (m_vkImageMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(VkGlobals::vkDevice, m_vkImageMemory, VkGlobals::vkAllocatorCallback);
        m_vkImageMemory = VK_NULL_HANDLE;
    }

    if (m_vkImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(VkGlobals::vkDevice, m_vkImage, VkGlobals::vkAllocatorCallback);
        m_vkImage = VK_NULL_HANDLE;
    }
}




Sampler::Sampler()
    : m_vkSampler(VK_NULL_HANDLE)
{
}

Sampler::Sampler(Sampler&& sampler) noexcept
    : m_vkSampler(sampler.m_vkSampler)
{
    sampler.m_vkSampler = VK_NULL_HANDLE;
}

Sampler::~Sampler()
{
    if (m_vkSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(VkGlobals::vkDevice, m_vkSampler, VkGlobals::vkAllocatorCallback);
        m_vkSampler = VK_NULL_HANDLE;
    }
}

Sampler& Sampler::operator=(Sampler && sampler) noexcept
{
    std::swap(m_vkSampler, sampler.m_vkSampler);
    return *this;
}

void Sampler::Create(ESampleFilter filter, ESampleMode mode, float anisotropy, float mipCount)
{
    VkSamplerCreateInfo samplerLinerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samplerLinerInfo.magFilter = to_vk_enum(filter);
    samplerLinerInfo.minFilter = to_vk_enum(filter);
    samplerLinerInfo.addressModeU = to_vk_enum(mode);
    samplerLinerInfo.addressModeV = to_vk_enum(mode);
    samplerLinerInfo.addressModeW = to_vk_enum(mode);
    samplerLinerInfo.anisotropyEnable = VK_FALSE;
    samplerLinerInfo.maxAnisotropy = anisotropy;
    samplerLinerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerLinerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerLinerInfo.compareEnable = VK_FALSE;
    samplerLinerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerLinerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerLinerInfo.mipLodBias = 0.0f;
    samplerLinerInfo.minLod = 0.0f;
    samplerLinerInfo.maxLod = mipCount;
    if (anisotropy > 1)
    {
        samplerLinerInfo.anisotropyEnable = VK_TRUE;
    }
    VK_ASSERT(vkCreateSampler(VkGlobals::vkDevice, &samplerLinerInfo, VkGlobals::vkAllocatorCallback, &m_vkSampler));
}