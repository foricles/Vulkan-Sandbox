#include "vkengine.hpp"
#include "vkutils.hpp"
#define NULL_QUEUE 0xFFFFFFFF

VkAllocationCallbacks*  VkGlobals::vkAllocatorCallback = nullptr;
VkInstance              VkGlobals::vkInstance = VK_NULL_HANDLE;
VkSurfaceKHR            VkGlobals::vkSurface = VK_NULL_HANDLE;
VkPhysicalDevice        VkGlobals::vkGPU = VK_NULL_HANDLE;
VkDevice                VkGlobals::vkDevice = VK_NULL_HANDLE;
VkCommandPool           VkGlobals::vkCommandPool = VK_NULL_HANDLE;
GpuQueue                VkGlobals::queue = { NULL_QUEUE, VK_NULL_HANDLE };
VkDescriptorPool        VkGlobals::vkDescriptorPool = VK_NULL_HANDLE;
VkQueryPool             VkGlobals::vkQueryPool = VK_NULL_HANDLE;
Swapchain               VkGlobals::swapchain = { 0, 0, 0, {}, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_NULL_HANDLE, {} };
double                  VulkanEngine::uGpuTimestampPeriod = 0;
bool                    VulkanEngine::bIsSwapchainCreated = false;
static                  VkDebugUtilsMessengerEXT g_pDebugger = VK_NULL_HANDLE;


#ifdef _DEBUG

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
);

#endif // _DEBUG



void VulkanEngine::InitInstance(const list& extensions, const list& layers)
{
    std::cout << "=== Init Instance ===" << std::endl;

    assert(IsLayersSupports(layers));
    assert(IsExtensionsSupports(extensions));

    VkApplicationInfo applicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    applicationInfo.pApplicationName = "Blur";
    applicationInfo.applicationVersion = 1;
    applicationInfo.pEngineName = "Ubisoft Problem";
    applicationInfo.engineVersion = 1;
    applicationInfo.apiVersion = VK_API_VERSION_1_3;



	VkInstanceCreateInfo instanceInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instanceInfo.pApplicationInfo = &applicationInfo;
    instanceInfo.ppEnabledLayerNames = layers.data();
    instanceInfo.ppEnabledExtensionNames = extensions.data();
    instanceInfo.enabledLayerCount = uint32_t(layers.size());
    instanceInfo.enabledExtensionCount = uint32_t(extensions.size());

#ifdef _DEBUG

    VkValidationFeatureEnableEXT enabledValidationFeatures[] = { VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT };
    VkValidationFeaturesEXT validationFeatures = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
    validationFeatures.enabledValidationFeatureCount = 1;
    validationFeatures.pEnabledValidationFeatures = enabledValidationFeatures;
    instanceInfo.pNext = &validationFeatures;

#endif

    VK_ASSERT(vkCreateInstance(&instanceInfo, VkGlobals::vkAllocatorCallback, &VkGlobals::vkInstance));

#ifdef _DEBUG

    std::cout << "=== Create debug messenger ===" << std::endl;

    VkDebugUtilsMessengerCreateInfoEXT debugInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugInfo.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
    debugInfo.pfnUserCallback = DebugCallback;

    PFN_vkCreateDebugUtilsMessengerEXT createDebugFoo = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(VkGlobals::vkInstance, "vkCreateDebugUtilsMessengerEXT");
    assert(createDebugFoo != nullptr);

    createDebugFoo(VkGlobals::vkInstance, &debugInfo, VkGlobals::vkAllocatorCallback, &g_pDebugger);

#endif // _DEBUG
}

void VulkanEngine::InitSurface(HWND hWnd, HINSTANCE hInstance, const list& devextensions, const list& devlayers)
{
    std::cout << "=== Create Surface ===" << std::endl;
    VkWin32SurfaceCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    createInfo.hwnd = static_cast<HWND>(hWnd);
    createInfo.hinstance = static_cast<HINSTANCE>(hInstance);

    VK_ASSERT(vkCreateWin32SurfaceKHR(VkGlobals::vkInstance, &createInfo, VkGlobals::vkAllocatorCallback, &VkGlobals::vkSurface));

    ChooseGpu(devextensions, devlayers);
    CreateDevice(devextensions, devlayers);
    CreatePools();
}

void VulkanEngine::ChooseGpu(const list& devextensions, const list& devlayers)
{
    std::cout << "=== Choosing GPU ===" << std::endl;
    std::vector<VkPhysicalDevice> devices;
    {
        uint32_t deviceCount = 0;
        VK_ASSERT(vkEnumeratePhysicalDevices(VkGlobals::vkInstance, &deviceCount, nullptr));
        devices.resize(deviceCount);
        VK_ASSERT(vkEnumeratePhysicalDevices(VkGlobals::vkInstance, &deviceCount, devices.data()));
    }

    for (VkPhysicalDevice physDevice : devices)
    {
        VkPhysicalDeviceFeatures deviceFeatures;
        VkPhysicalDeviceProperties deviceProperties;
        std::vector<VkQueueFamilyProperties> queueFamilies;

        {
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, nullptr);
            queueFamilies.resize(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, queueFamilies.data());
            vkGetPhysicalDeviceFeatures(physDevice, &deviceFeatures);
            vkGetPhysicalDeviceProperties(physDevice, &deviceProperties);
        }

        if (!deviceFeatures.samplerAnisotropy)
        {
            continue;
        }

        std::cout << " gpu: " << deviceProperties.deviceName << std::endl;
        if (!IsDeviceLayersSupports(physDevice, devlayers))
        {
            continue;
        }
        if (!IsDeviceExtensionsSupports(physDevice, devextensions))
        {
            continue;
        }

        for (size_t i(0); i < queueFamilies.size(); ++i)
        {
            if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                && (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
                && (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT))
            {
                VkBool32 bSupportPresent = VK_FALSE;
                VK_ASSERT(vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, (uint32_t)i, VkGlobals::vkSurface, &bSupportPresent));
                if (bSupportPresent)
                {
                    VkGlobals::queue.familyIndex = (uint32_t)i;
                    break;
                }
            }
        }

        if (VkGlobals::queue.familyIndex != NULL_QUEUE)
        {
            VkGlobals::vkGPU = physDevice;
            uGpuTimestampPeriod = deviceProperties.limits.timestampPeriod;
            break;
        }
    }
}

void VulkanEngine::CreateDevice(const list& devextensions, const list& devlayers)
{
    std::cout << "=== Create Device ===" << std::endl;

    float fPriority = 1.0f;
    VkDeviceQueueCreateInfo deviceQueueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    deviceQueueInfo.queueFamilyIndex = VkGlobals::queue.familyIndex;
    deviceQueueInfo.queueCount = 1;
    deviceQueueInfo.pQueuePriorities = &fPriority;

    VkPhysicalDeviceBufferDeviceAddressFeatures bufferAddress = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES };
    bufferAddress.bufferDeviceAddress = VK_TRUE;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR raytraccingFeature = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
    raytraccingFeature.rayTracingPipeline = VK_TRUE;
    raytraccingFeature.pNext = &bufferAddress;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationFeature = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
    accelerationFeature.accelerationStructure = VK_TRUE;
    accelerationFeature.pNext = &raytraccingFeature;

    VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures separateDepthStencil = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES };
    separateDepthStencil.separateDepthStencilLayouts = VK_TRUE;
    separateDepthStencil.pNext = &accelerationFeature;

    VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures demoteFeature = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES };
    demoteFeature.shaderDemoteToHelperInvocation = VK_TRUE;
    demoteFeature.pNext = &separateDepthStencil;

    VkPhysicalDeviceSynchronization2Features synchronization2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES };
    synchronization2.synchronization2 = VK_TRUE;
    synchronization2.pNext = &demoteFeature;

    VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    physicalDeviceFeatures2.features.samplerAnisotropy = VK_TRUE;
    physicalDeviceFeatures2.pNext = &synchronization2;

    VkDeviceCreateInfo deviceInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &deviceQueueInfo;
    deviceInfo.enabledLayerCount = uint32_t(devlayers.size());
    deviceInfo.enabledExtensionCount = uint32_t(devextensions.size());
    deviceInfo.ppEnabledLayerNames = devlayers.data();
    deviceInfo.ppEnabledExtensionNames = devextensions.data();
    deviceInfo.pNext = &physicalDeviceFeatures2;

    VK_ASSERT(vkCreateDevice(VkGlobals::vkGPU, &deviceInfo, VkGlobals::vkAllocatorCallback, &VkGlobals::vkDevice));

    vkGetDeviceQueue(VkGlobals::vkDevice, VkGlobals::queue.familyIndex, 0, &VkGlobals::queue.vkQueue);
}

void VulkanEngine::CreatePools()
{
    // command pool
    VkCommandPoolCreateInfo cmdPoolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmdPoolInfo.queueFamilyIndex = VkGlobals::queue.familyIndex;
    VK_ASSERT(vkCreateCommandPool(VkGlobals::vkDevice, &cmdPoolInfo, VkGlobals::vkAllocatorCallback, &VkGlobals::vkCommandPool));


    //descriptors pool
    std::vector<VkDescriptorPoolSize> sizes = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 2},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 5},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}
    };

    VkDescriptorPoolCreateInfo descriptorPoolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptorPoolInfo.maxSets = 500;
    descriptorPoolInfo.poolSizeCount = uint32_t(sizes.size());
    descriptorPoolInfo.pPoolSizes = sizes.data();

    VK_ASSERT(vkCreateDescriptorPool(VkGlobals::vkDevice, &descriptorPoolInfo, VkGlobals::vkAllocatorCallback, &VkGlobals::vkDescriptorPool));


    // query pool
    VkQueryPool queryPool = VK_NULL_HANDLE;
    VkQueryPoolCreateInfo queryPoolInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
    queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    queryPoolInfo.queryCount = kQueryCount;
    VK_ASSERT(vkCreateQueryPool(VkGlobals::vkDevice, &queryPoolInfo, VkGlobals::vkAllocatorCallback, &VkGlobals::vkQueryPool));
}

void VulkanEngine::UpdateSwapchain(uint32_t width, uint32_t height)
{
    VkSurfaceCapabilitiesKHR capabilities;
    VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkGlobals::vkGPU, VkGlobals::vkSurface, &capabilities));

    VkGlobals::swapchain.width = std::max(std::min(width, capabilities.maxImageExtent.width), capabilities.minImageExtent.width);
    VkGlobals::swapchain.height = std::max(std::min(height, capabilities.maxImageExtent.height), capabilities.minImageExtent.height);

    if (bIsSwapchainCreated)
    {
        vkDeviceWaitIdle(VkGlobals::vkDevice);
        DestroySwapchain();
    }
    else
    {
        std::cout << "=== Create Swapchain ===" << std::endl;
        bIsSwapchainCreated = true;

        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
        {
            uint32_t formatsCount = 0;
            uint32_t presentCount = 0;
            vkGetPhysicalDeviceSurfaceFormatsKHR(VkGlobals::vkGPU, VkGlobals::vkSurface, &formatsCount, nullptr);
            vkGetPhysicalDeviceSurfacePresentModesKHR(VkGlobals::vkGPU, VkGlobals::vkSurface, &presentCount, nullptr);
            formats.resize(formatsCount);
            presentModes.resize(presentCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(VkGlobals::vkGPU, VkGlobals::vkSurface, &formatsCount, formats.data());
            vkGetPhysicalDeviceSurfacePresentModesKHR(VkGlobals::vkGPU, VkGlobals::vkSurface, &presentCount, presentModes.data());
        }

        const auto isFormatSupported = [&formats](const VkSurfaceFormatKHR& format)->bool {
            const auto cmp = [format](const VkSurfaceFormatKHR& formatlist) {
                return format.format == formatlist.format && format.colorSpace == formatlist.colorSpace;
            };
            return std::find_if(formats.begin(), formats.end(), cmp) != formats.end();
        };

        VkGlobals::swapchain.format = formats[0];
        if (isFormatSupported({ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }))
        {
            VkGlobals::swapchain.format.format = VK_FORMAT_R8G8B8A8_UNORM;
            VkGlobals::swapchain.format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
            std::cout << " chosen format " << "R8G8B8A8_UNORM" << " color space " << "SRGB_NONLINEAR" << std::endl;
        }
        else if (isFormatSupported({ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }))
        {
            VkGlobals::swapchain.format.format = VK_FORMAT_B8G8R8A8_UNORM;
            VkGlobals::swapchain.format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
            std::cout << " chosen format " << "B8G8R8A8_UNORM" << " color space " << "SRGB_NONLINEAR" << std::endl;
        }
        else if (isFormatSupported({ VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }))
        {
            VkGlobals::swapchain.format.format = VK_FORMAT_R8G8B8A8_SRGB;
            VkGlobals::swapchain.format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
            std::cout << " chosen format " << "R8G8B8A8_SRGB" << " color space " << "SRGB_NONLINEAR" << std::endl;
        }
        else if (isFormatSupported({ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }))
        {
            VkGlobals::swapchain.format.format = VK_FORMAT_B8G8R8A8_SRGB;
            VkGlobals::swapchain.format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
            std::cout << " chosen format " << "B8G8R8A8_SRGB" << " color space " << "SRGB_NONLINEAR" << std::endl;
        }

        const auto isPresentModeSupported = [&presentModes](const VkPresentModeKHR& mode)->bool {
            const auto cmp = [mode](const VkPresentModeKHR& modeList) {
                return modeList == mode;
            };
            return std::find_if(presentModes.begin(), presentModes.end(), cmp) != presentModes.end();
        };

        VkGlobals::swapchain.presentMode = presentModes[0];
        if (isPresentModeSupported(VK_PRESENT_MODE_MAILBOX_KHR))
        {
            VkGlobals::swapchain.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            std::cout << " chosen mode: " << "MAILBOX" << std::endl;
        }
        else if (isPresentModeSupported(VK_PRESENT_MODE_FIFO_RELAXED_KHR))
        {
            VkGlobals::swapchain.presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
            std::cout << " chosen mode: " << "RELAXED" << std::endl;
        }
        else if (isPresentModeSupported(VK_PRESENT_MODE_FIFO_KHR))
        {
            VkGlobals::swapchain.presentMode = VK_PRESENT_MODE_FIFO_KHR;
            std::cout << " chosen mode: " << "FIFO" << std::endl;
        }
        else if (isPresentModeSupported(VK_PRESENT_MODE_IMMEDIATE_KHR))
        {
            VkGlobals::swapchain.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            std::cout << " chosen mode: " << "IMMEDIATE" << std::endl;
        }
    }

    VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    createInfo.surface = VkGlobals::vkSurface;
    createInfo.minImageCount = VulkanEngine::kSwapchainImageCount;
    createInfo.imageExtent.width = VkGlobals::swapchain.width;
    createInfo.imageExtent.height = VkGlobals::swapchain.height;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    createInfo.presentMode = VkGlobals::swapchain.presentMode;
    createInfo.imageFormat = VkGlobals::swapchain.format.format;
    createInfo.imageColorSpace = VkGlobals::swapchain.format.colorSpace;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.pQueueFamilyIndices = 0;
    createInfo.queueFamilyIndexCount = 0;

    VK_ASSERT(
        vkCreateSwapchainKHR(
            VkGlobals::vkDevice,
            &createInfo,
            VkGlobals::vkAllocatorCallback,
            &VkGlobals::swapchain.vkSwapchain
        )
    );

    uint32_t uImageCount = 0;
    vkGetSwapchainImagesKHR(VkGlobals::vkDevice, VkGlobals::swapchain.vkSwapchain, &uImageCount, nullptr);
    assert(VulkanEngine::kSwapchainImageCount == uImageCount);

    VkGlobals::swapchain.images.resize(VulkanEngine::kSwapchainImageCount);
    vkGetSwapchainImagesKHR(VkGlobals::vkDevice, VkGlobals::swapchain.vkSwapchain, &uImageCount, VkGlobals::swapchain.images.data());


    VkGlobals::swapchain.views.resize(VulkanEngine::kSwapchainImageCount);

    for (uint32_t i(0); i < VulkanEngine::kSwapchainImageCount; ++i)
    {
        VkImageViewCreateInfo imageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        imageViewCreateInfo.image = VkGlobals::swapchain.images[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = VkGlobals::swapchain.format.format;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        VK_ASSERT(
            vkCreateImageView(
                VkGlobals::vkDevice,
                &imageViewCreateInfo,
                VkGlobals::vkAllocatorCallback,
                &VkGlobals::swapchain.views[i]
            )
        );
    }

    ++VkGlobals::swapchain.generation;
}

void VulkanEngine::DestroySwapchain()
{
    for (VkImageView& imageView : VkGlobals::swapchain.views)
    {
        if (imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(VkGlobals::vkDevice, imageView, VkGlobals::vkAllocatorCallback);
        }
        imageView = VK_NULL_HANDLE;
    }
    if (VkGlobals::swapchain.vkSwapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(VkGlobals::vkDevice, VkGlobals::swapchain.vkSwapchain, VkGlobals::vkAllocatorCallback);
    }
    VkGlobals::swapchain.vkSwapchain = VK_NULL_HANDLE;
}

void VulkanEngine::Shutdown()
{
    vkDestroyQueryPool(VkGlobals::vkDevice, VkGlobals::vkQueryPool, VkGlobals::vkAllocatorCallback);
    vkDestroyDescriptorPool(VkGlobals::vkDevice, VkGlobals::vkDescriptorPool, VkGlobals::vkAllocatorCallback);
    vkDestroyCommandPool(VkGlobals::vkDevice, VkGlobals::vkCommandPool, VkGlobals::vkAllocatorCallback);

    DestroySwapchain();
    vkDestroySurfaceKHR(VkGlobals::vkInstance, VkGlobals::vkSurface, VkGlobals::vkAllocatorCallback);
    vkDestroyDevice(VkGlobals::vkDevice, VkGlobals::vkAllocatorCallback);

#ifdef _DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugUtilsMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(VkGlobals::vkInstance, "vkDestroyDebugUtilsMessengerEXT");
    assert(destroyDebugUtilsMessenger != nullptr);
    destroyDebugUtilsMessenger(VkGlobals::vkInstance, g_pDebugger, VkGlobals::vkAllocatorCallback);
#endif // _DEBUG

    vkDestroyInstance(VkGlobals::vkInstance, VkGlobals::vkAllocatorCallback);
}

VkFence VulkanEngine::CreateFence()
{
    VkFence fence = VK_NULL_HANDLE;
    VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    VK_ASSERT(vkCreateFence(VkGlobals::vkDevice, &createInfo, VkGlobals::vkAllocatorCallback, &fence));
    return fence;
}

VkSemaphore VulkanEngine::CreateVkSemaphore()
{
    VkSemaphore pSemaphore = VK_NULL_HANDLE;
    VkSemaphoreCreateInfo pInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VK_ASSERT(vkCreateSemaphore(VkGlobals::vkDevice, &pInfo, VkGlobals::vkAllocatorCallback, &pSemaphore));
    return pSemaphore;
}

VkCommandBuffer VulkanEngine::CreateCommandBuffer()
{
    VkCommandBuffer pCommandBuffer = VK_NULL_HANDLE;
    VkCommandBufferAllocateInfo pAllocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    pAllocInfo.commandPool = VkGlobals::vkCommandPool;
    pAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    pAllocInfo.commandBufferCount = 1;

    VK_ASSERT(vkAllocateCommandBuffers(VkGlobals::vkDevice, &pAllocInfo, &pCommandBuffer));

    return pCommandBuffer;
}

void VulkanEngine::DestroyFence(VkFence& vkFence)
{
    vkDestroyFence(VkGlobals::vkDevice, vkFence, VkGlobals::vkAllocatorCallback);
    vkFence = VK_NULL_HANDLE;
}

void VulkanEngine::DestroyVkSemaphore(VkSemaphore& vkSemaphore)
{
    vkDestroySemaphore(VkGlobals::vkDevice, vkSemaphore, VkGlobals::vkAllocatorCallback);
    vkSemaphore = VK_NULL_HANDLE;
}

void VulkanEngine::DestroyCommandBuffer(VkCommandBuffer& vkCommandBuffer)
{
    vkFreeCommandBuffers(VkGlobals::vkDevice, VkGlobals::vkCommandPool, 1, &vkCommandBuffer);
    vkCommandBuffer = VK_NULL_HANDLE;
}

VkDeviceMemory VulkanEngine::AllocateMemory(VkBuffer vkBuffer, VkMemoryPropertyFlags properties, VkMemoryAllocateFlagBits flags)
{
    VkMemoryRequirements requirements = {};
    vkGetBufferMemoryRequirements(VkGlobals::vkDevice, vkBuffer, &requirements);

    VkMemoryAllocateFlagsInfo allocateFlags = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO };
    allocateFlags.flags = flags;

    VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = requirements.size;

    if (flags != VK_MEMORY_ALLOCATE_FLAG_BITS_MAX_ENUM)
    {
        allocInfo.pNext = &allocateFlags;
    }

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(VkGlobals::vkGPU, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if (requirements.memoryTypeBits & (1 << i))
        {
            if ((memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                allocInfo.memoryTypeIndex = i;
                break;
            }
        }
    }

    VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
    VK_ASSERT(vkAllocateMemory(VkGlobals::vkDevice, &allocInfo, VkGlobals::vkAllocatorCallback, &deviceMemory));

    return deviceMemory;
}

VkDeviceMemory VulkanEngine::AllocateMemory(VkImage vkImage, VkMemoryPropertyFlags properties)
{
    VkMemoryRequirements requirements = {};
    vkGetImageMemoryRequirements(VkGlobals::vkDevice, vkImage, &requirements);

    VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = requirements.size;

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(VkGlobals::vkGPU, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if (requirements.memoryTypeBits & (1 << i))
        {
            if ((memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                allocInfo.memoryTypeIndex = i;
                break;
            }
        }
    }

    VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
    VK_ASSERT(vkAllocateMemory(VkGlobals::vkDevice, &allocInfo, VkGlobals::vkAllocatorCallback, &deviceMemory));

    return deviceMemory;
}

void VulkanEngine::SubmitOnce(std::function<void(VkCommandBuffer)> callback, VkCommandPool commandPool)
{
    if (commandPool == VK_NULL_HANDLE)
    {
        commandPool = VkGlobals::vkCommandPool;
    }

    // allocate command buffer
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

    VkCommandBufferAllocateInfo bufferInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    bufferInfo.commandPool = commandPool;
    bufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bufferInfo.commandBufferCount = 1;
    VK_ASSERT(vkAllocateCommandBuffers(VkGlobals::vkDevice, &bufferInfo, &commandBuffer));

    // begin buffer once
    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_ASSERT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    // do job
    callback(commandBuffer);

    // submit command
    VK_ASSERT(vkEndCommandBuffer(commandBuffer));

    VkFence fence = VulkanEngine::CreateFence();
    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    VK_ASSERT(vkQueueSubmit(VkGlobals::queue.vkQueue, 1, &submitInfo, fence));

    // wait to complete
    VK_ASSERT(vkWaitForFences(VkGlobals::vkDevice, 1, &fence, VK_TRUE, UINT64_MAX));

    //clear
    vkDestroyFence(VkGlobals::vkDevice, fence, VkGlobals::vkAllocatorCallback);
    vkFreeCommandBuffers(VkGlobals::vkDevice, commandPool, 1, &commandBuffer);
}











bool VulkanEngine::IsLayersSupports(const list& layers)
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    bool isSupport = true;
    for (auto& layer : layers)
    {
        const auto cmp = [layer](const VkLayerProperties& props) {
            return ::strcmp(layer, props.layerName) == 0;
        };

        const auto fnd = std::find_if(availableLayers.begin(), availableLayers.end(), cmp);
        if (fnd == availableLayers.end())
        {
            isSupport = false;
            std::cout << " Layer not supported: " << layer << std::endl;
        }
    }

    return isSupport;
}

bool VulkanEngine::IsExtensionsSupports(const list& extensions)
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensionsSupported(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionsSupported.data());

    bool isSupport = true;
    for (auto& ext : extensions)
    {
        const auto cmp = [ext](const VkExtensionProperties& props) {
            return ::strcmp(ext, props.extensionName) == 0;
        };

        const auto fnd = std::find_if(extensionsSupported.begin(), extensionsSupported.end(), cmp);
        if (fnd == extensionsSupported.end())
        {
            isSupport = false;
            std::cout << " Extensions not supported: " << ext << std::endl;
        }
    }

    return isSupport;
}

bool VulkanEngine::IsDeviceLayersSupports(VkPhysicalDevice phd, const list& layers)
{
    uint32_t count = 0;
    vkEnumerateDeviceLayerProperties(phd, &count, nullptr);
    std::vector<VkLayerProperties> availableProps(count);
    vkEnumerateDeviceLayerProperties(phd, &count, availableProps.data());

    bool isSupports = true;
    for (auto& layer : layers)
    {
        const auto cmp = [layer](const VkLayerProperties& props) {
            return ::strcmp(layer, props.layerName) == 0;
        };

        const auto fnd = std::find_if(availableProps.begin(), availableProps.end(), cmp);
        if (fnd == availableProps.end())
        {
            isSupports = false;
            std::cout << " Device layer not supported: " << layer << std::endl;
        }
    }

    return isSupports;
}

bool VulkanEngine::IsDeviceExtensionsSupports(VkPhysicalDevice phd, const list& extensions)
{
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(phd, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> availableProps(count);
    vkEnumerateDeviceExtensionProperties(phd, nullptr, &count, availableProps.data());

    bool isSupports = true;
    for (auto& ext : extensions)
    {
        const auto cmp = [ext](const VkExtensionProperties& props) {
            return ::strcmp(ext, props.extensionName) == 0;
        };

        const auto fnd = std::find_if(availableProps.begin(), availableProps.end(), cmp);
        if (fnd == availableProps.end())
        {
            isSupports = false;
            std::cout << " Device extension not supported: " << ext << std::endl;
        }
    }

    return isSupports;
}

double VulkanEngine::GetGpuTimestampPeriod()
{
    return uGpuTimestampPeriod;
}

#ifdef _DEBUG

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    std::cout << "DebugCallback: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

#endif // _DEBUG