#include "vkbuffer.hpp"
#include "vkutils.hpp"


Buffer::Buffer(EBufferType eType, bool cpuCoherent)
    : m_eType(eType)
    , m_size(0)
    , m_vkBuffer(VK_NULL_HANDLE)
    , m_vkMemory(VK_NULL_HANDLE)
    , m_isCpuCoherent(cpuCoherent)
{
}

Buffer::Buffer(Buffer&& buff) noexcept
    : m_eType(buff.m_eType)
    , m_size(buff.m_size)
    , m_vkBuffer(buff.m_vkBuffer)
    , m_vkMemory(buff.m_vkMemory)
    , m_isCpuCoherent(buff.m_isCpuCoherent)
{
    buff.m_size = 0;
    buff.m_vkBuffer = VK_NULL_HANDLE;
    buff.m_vkMemory = VK_NULL_HANDLE;
}

Buffer::~Buffer()
{
    FreeBuffer();
}

Buffer& Buffer::operator=(Buffer&& buff) noexcept
{
    std::swap(m_eType, buff.m_eType);
    std::swap(m_size, buff.m_size);
    std::swap(m_vkBuffer, buff.m_vkBuffer);
    std::swap(m_vkMemory, buff.m_vkMemory);
    std::swap(m_isCpuCoherent, buff.m_isCpuCoherent);
	return *this;
}

void Buffer::Load(const void* pMem, uint32_t size, VkCommandPool commandPool)
{
    if (size != m_size)
    {
        FreeBuffer();
    }

    if (m_vkBuffer == VK_NULL_HANDLE)
    {
        m_size = size;
        VkBufferCreateInfo vbufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        vbufferInfo.size = m_size;
        vbufferInfo.usage = to_vk_enum(m_eType);
        vbufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (!m_isCpuCoherent)
        {
            vbufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        }

        VK_ASSERT(vkCreateBuffer(VkGlobals::vkDevice, &vbufferInfo, VkGlobals::vkAllocatorCallback, &m_vkBuffer));
    }

    if (m_vkMemory == VK_NULL_HANDLE)
    {
        if (m_isCpuCoherent)
        {
            m_vkMemory = VulkanEngine::AllocateMemory(m_vkBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        }
        else
        {
            m_vkMemory = VulkanEngine::AllocateMemory(m_vkBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);
        }
        VK_ASSERT(vkBindBufferMemory(VkGlobals::vkDevice, m_vkBuffer, m_vkMemory, 0));
    }

    if (m_isCpuCoherent)
    {
        // copy data
        void* pMapedMemory = nullptr;
        VK_ASSERT(vkMapMemory(VkGlobals::vkDevice, m_vkMemory, 0, size, 0, &pMapedMemory));
        ::memcpy(pMapedMemory, pMem, size);
        vkUnmapMemory(VkGlobals::vkDevice, m_vkMemory);
    }
    else
    {

        // allocate staging
        VkBuffer vkStagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vkStagingMemory = VK_NULL_HANDLE;

        VkBufferCreateInfo vbufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        vbufferInfo.size = m_size;
        vbufferInfo.usage = to_vk_enum(m_eType) | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        vbufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_ASSERT(vkCreateBuffer(VkGlobals::vkDevice, &vbufferInfo, VkGlobals::vkAllocatorCallback, &vkStagingBuffer));

        vkStagingMemory = VulkanEngine::AllocateMemory(vkStagingBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        VK_ASSERT(vkBindBufferMemory(VkGlobals::vkDevice, vkStagingBuffer, vkStagingMemory, 0));

        // copy data to staging
        void* pMapedMemory = nullptr;
        VK_ASSERT(vkMapMemory(VkGlobals::vkDevice, vkStagingMemory, 0, size, 0, &pMapedMemory));
        ::memcpy(pMapedMemory, pMem, size);
        vkUnmapMemory(VkGlobals::vkDevice, vkStagingMemory);

        // submit to gpu
        VulkanEngine::SubmitOnce([&](VkCommandBuffer commandBuffer) {
            VkBufferCopy copyRegion = {};
            copyRegion.size = size;
            vkCmdCopyBuffer(commandBuffer, vkStagingBuffer, m_vkBuffer, 1, &copyRegion);
            }, commandPool);


        //clear
        vkFreeMemory(VkGlobals::vkDevice, vkStagingMemory, VkGlobals::vkAllocatorCallback);
        vkDestroyBuffer(VkGlobals::vkDevice, vkStagingBuffer, VkGlobals::vkAllocatorCallback);
    }
}

VkDescriptorBufferInfo Buffer::GetDscInfo() const
{
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = m_vkBuffer;
    bufferInfo.range = m_size;
    bufferInfo.offset = 0;
    return bufferInfo;
}

VkDeviceAddress Buffer::GetDeviceAddress() const
{
    VkBufferDeviceAddressInfo pInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
    pInfo.buffer = m_vkBuffer;
    VkDeviceAddress address = vkGetBufferDeviceAddress(VkGlobals::vkDevice, &pInfo);
    return address;
}

void Buffer::FreeBuffer()
{
    if (m_vkMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(VkGlobals::vkDevice, m_vkMemory, VkGlobals::vkAllocatorCallback);
        m_vkMemory = VK_NULL_HANDLE;
    }

    if (m_vkBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(VkGlobals::vkDevice, m_vkBuffer, VkGlobals::vkAllocatorCallback);
        m_vkBuffer = VK_NULL_HANDLE;
    }
}