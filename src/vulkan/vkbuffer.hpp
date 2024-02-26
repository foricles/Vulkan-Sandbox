#pragma once
#include "vkengine.hpp"
#include "vkcommon.hpp"


class Buffer
{
public:
	Buffer(EBufferType eType, bool cpuCoherent = false);
    Buffer(Buffer&& buff) noexcept;
	~Buffer();

    inline operator VkBuffer& () { return m_vkBuffer; }
    Buffer& operator=(Buffer&& buff) noexcept;

    void Load(const void* pMem, uint32_t size, VkCommandPool commandPool = VK_NULL_HANDLE);
    VkDescriptorBufferInfo GetDscInfo() const;

public:
    inline uint32_t size() const { return m_size; }
    inline VkDeviceMemory GetMemory() const { return m_vkMemory; }

public:
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

private:
    void FreeBuffer();

private:
    EBufferType m_eType;
    uint32_t m_size;
    VkBuffer m_vkBuffer;
    VkDeviceMemory m_vkMemory;
    bool m_isCpuCoherent;

public:
    template<class T>
    void Load(const std::vector<T>& source, VkCommandPool commandPool = VK_NULL_HANDLE)
    {
        constexpr uint32_t dSize = sizeof(T);
        Load(static_cast<const void*>(source.data()), static_cast<uint32_t>(source.size() * dSize), commandPool);
    }
};