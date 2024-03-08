#pragma once
#include <vulkan/vulkan.h>
#include <functional>
#include <Windows.h>
#include <vector>

struct GpuQueue
{
	uint32_t familyIndex;
	VkQueue vkQueue;
};

struct Swapchain
{
	uint32_t width;
	uint32_t height;
	uint64_t generation;
	VkSurfaceFormatKHR format;
	VkPresentModeKHR presentMode;
	VkSwapchainKHR vkSwapchain;
	std::vector<VkImage> images;
	std::vector<VkImageView> views;
};

struct VkGlobals
{
	static VkAllocationCallbacks* vkAllocatorCallback;
	static VkInstance vkInstance;
	static VkSurfaceKHR vkSurface;
	static VkPhysicalDevice vkGPU;
	static GpuQueue queue;
	static VkDevice vkDevice;
	static Swapchain swapchain;
	static VkCommandPool vkCommandPool;
	static VkDescriptorPool vkDescriptorPool;
	static VkQueryPool vkQueryPool;
};

class VulkanEngine
{
public:
	using list = std::vector<const char*>;
	static constexpr uint32_t kQueryCount = 2;
	static constexpr uint32_t kSwapchainImageCount = 2;

public:
	static void InitInstance(const list& extensions, const list& layers);
	static void InitSurface(HWND hWnd, HINSTANCE hInstance, const list& devextensions, const list& devlayers);
	static void UpdateSwapchain(uint32_t width, uint32_t height);
	static void Shutdown();

	static VkFence CreateFence();
	static void DestroyFence(VkFence& vkFence);

	static VkSemaphore CreateVkSemaphore();
	static void DestroyVkSemaphore(VkSemaphore& vkSemaphore);

	static VkCommandBuffer CreateCommandBuffer();
	static void DestroyCommandBuffer(VkCommandBuffer& vkCommandBuffer);

	static VkDeviceMemory AllocateMemory(VkBuffer vkBuffer, VkMemoryPropertyFlags properties, VkMemoryAllocateFlagBits flags = VK_MEMORY_ALLOCATE_FLAG_BITS_MAX_ENUM);
	static VkDeviceMemory AllocateMemory(VkImage vkImage, VkMemoryPropertyFlags properties);
	static void SubmitOnce(std::function<void(VkCommandBuffer)> callback, VkCommandPool commandPool = VK_NULL_HANDLE);

	static double GetGpuTimestampPeriod();

private:
	static void ChooseGpu(const list& devextensions, const list& devlayers);
	static void CreateDevice(const list& devextensions, const list& devlayers);
	static void CreatePools();
	static void DestroySwapchain();
	static bool IsLayersSupports(const list& layers);
	static bool IsExtensionsSupports(const list& extensions);
	static bool IsDeviceLayersSupports(VkPhysicalDevice phd, const list& layers);
	static bool IsDeviceExtensionsSupports(VkPhysicalDevice phd, const list& extensions);

private:
	static bool bIsSwapchainCreated;
	static double uGpuTimestampPeriod;
};