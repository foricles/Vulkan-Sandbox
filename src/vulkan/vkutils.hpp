#pragma once
#include <iostream>
#include <vulkan/vulkan.h>
#include <cassert>
#include "vkcommon.hpp"

#define ALM_LOG_VK_ERROR(VkError) case VkError: std::cout << "VK_ERROR: " << #VkError << std::endl; break


inline void PrintVkError(VkResult result)
{
	switch (result)
	{
		ALM_LOG_VK_ERROR(VK_NOT_READY);
		ALM_LOG_VK_ERROR(VK_TIMEOUT);
		ALM_LOG_VK_ERROR(VK_EVENT_SET);
		ALM_LOG_VK_ERROR(VK_EVENT_RESET);
		ALM_LOG_VK_ERROR(VK_INCOMPLETE);
		ALM_LOG_VK_ERROR(VK_ERROR_OUT_OF_HOST_MEMORY);
		ALM_LOG_VK_ERROR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
		ALM_LOG_VK_ERROR(VK_ERROR_INITIALIZATION_FAILED);
		ALM_LOG_VK_ERROR(VK_ERROR_DEVICE_LOST);
		ALM_LOG_VK_ERROR(VK_ERROR_MEMORY_MAP_FAILED);
		ALM_LOG_VK_ERROR(VK_ERROR_LAYER_NOT_PRESENT);
		ALM_LOG_VK_ERROR(VK_ERROR_EXTENSION_NOT_PRESENT);
		ALM_LOG_VK_ERROR(VK_ERROR_FEATURE_NOT_PRESENT);
		ALM_LOG_VK_ERROR(VK_ERROR_INCOMPATIBLE_DRIVER);
		ALM_LOG_VK_ERROR(VK_ERROR_TOO_MANY_OBJECTS);
		ALM_LOG_VK_ERROR(VK_ERROR_FORMAT_NOT_SUPPORTED);
		ALM_LOG_VK_ERROR(VK_ERROR_FRAGMENTED_POOL);
		ALM_LOG_VK_ERROR(VK_ERROR_UNKNOWN);
		ALM_LOG_VK_ERROR(VK_ERROR_OUT_OF_POOL_MEMORY);
		ALM_LOG_VK_ERROR(VK_ERROR_INVALID_EXTERNAL_HANDLE);
		ALM_LOG_VK_ERROR(VK_ERROR_FRAGMENTATION);
		ALM_LOG_VK_ERROR(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS);
		ALM_LOG_VK_ERROR(VK_PIPELINE_COMPILE_REQUIRED);
		ALM_LOG_VK_ERROR(VK_ERROR_SURFACE_LOST_KHR);
		ALM_LOG_VK_ERROR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
		ALM_LOG_VK_ERROR(VK_SUBOPTIMAL_KHR);
		ALM_LOG_VK_ERROR(VK_ERROR_OUT_OF_DATE_KHR);
		ALM_LOG_VK_ERROR(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
		ALM_LOG_VK_ERROR(VK_ERROR_VALIDATION_FAILED_EXT);
		ALM_LOG_VK_ERROR(VK_ERROR_INVALID_SHADER_NV);
		ALM_LOG_VK_ERROR(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
		ALM_LOG_VK_ERROR(VK_ERROR_NOT_PERMITTED_KHR);
		ALM_LOG_VK_ERROR(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
		ALM_LOG_VK_ERROR(VK_THREAD_IDLE_KHR);
		ALM_LOG_VK_ERROR(VK_THREAD_DONE_KHR);
		ALM_LOG_VK_ERROR(VK_OPERATION_DEFERRED_KHR);
		ALM_LOG_VK_ERROR(VK_OPERATION_NOT_DEFERRED_KHR);
		ALM_LOG_VK_ERROR(VK_ERROR_COMPRESSION_EXHAUSTED_EXT);
		ALM_LOG_VK_ERROR(VK_RESULT_MAX_ENUM);
#ifdef VK_ENABLE_BETA_EXTENSIONS
		ALM_LOG_VK_ERROR(VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR);
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
		ALM_LOG_VK_ERROR(VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR);
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
		ALM_LOG_VK_ERROR(VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR);
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
		ALM_LOG_VK_ERROR(VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR);
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
		ALM_LOG_VK_ERROR(VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR);
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
		ALM_LOG_VK_ERROR(VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR);
#endif
	default:
		break;
	}
}

#define VK_ASSERT(call) \
	do {\
		VkResult result = call; \
		PrintVkError(result); \
		assert(result == VK_SUCCESS);\
	} while(0)


template<class T> 
inline auto to_vk_enum(T e)
{
	static_assert(false, "to_vk_enum not implemented");
	return { 0 };
}

template<>
inline auto to_vk_enum(EShaderType e)
{
	switch (e)
	{
	case EShaderType::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
	case EShaderType::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
	case EShaderType::RayGeneration: return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	case EShaderType::RayMiss: return VK_SHADER_STAGE_MISS_BIT_KHR;
	case EShaderType::RayClosestHit: return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	default: return VK_SHADER_STAGE_VERTEX_BIT;
	}
}

template<>
inline auto to_vk_enum(EBufferType e)
{
	switch (e)
	{
	case EBufferType::Vertex: return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	case EBufferType::Index: return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	case EBufferType::Uniform: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	default: return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	}
}

template<>
inline auto to_vk_enum(EDescriptorType e)
{
	switch (e)
	{
	case EDescriptorType::Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
	case EDescriptorType::SampledImage: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	case EDescriptorType::StorageImage: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	case EDescriptorType::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	case EDescriptorType::InputAttachment: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	case EDescriptorType::AccelerationStructure: return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	default: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	}
}

template<>
inline auto to_vk_enum(ETopology e)
{
	switch (e)
	{
	case ETopology::Point: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	case ETopology::Line: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	default: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}
}

template<>
inline auto to_vk_enum(EFillMode e)
{
	switch (e)
	{
	case EFillMode::Line: return VK_POLYGON_MODE_LINE;
	case EFillMode::Point: return VK_POLYGON_MODE_POINT;
	default: return VK_POLYGON_MODE_FILL;
	}
}

template<>
inline auto to_vk_enum(ECull e)
{
	switch (e)
	{
	case ECull::Front: return VK_CULL_MODE_FRONT_BIT;
	case ECull::Back: return VK_CULL_MODE_BACK_BIT;
	case ECull::FrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;
	default: return VK_CULL_MODE_NONE;
	}
}

template<>
inline auto to_vk_enum(EFrontFace e)
{
	switch (e)
	{
	case EFrontFace::CW: return VK_FRONT_FACE_CLOCKWISE;
	default: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
	}
}

template<>
inline auto to_vk_enum(EDepthFunc e)
{
	switch (e)
	{
	case EDepthFunc::Less: return VK_COMPARE_OP_LESS;
	case EDepthFunc::Equal: return VK_COMPARE_OP_EQUAL;
	case EDepthFunc::LessEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
	case EDepthFunc::Greater: return VK_COMPARE_OP_GREATER;
	case EDepthFunc::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
	case EDepthFunc::GreaterEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case EDepthFunc::Allways: return VK_COMPARE_OP_ALWAYS;
	default: return VK_COMPARE_OP_NEVER;
	}
}

template<>
inline auto to_vk_enum(EPixelFormat almEnum)
{
	switch (almEnum)
	{
	case EPixelFormat::Mono: return VK_FORMAT_R32_SFLOAT;
	case EPixelFormat::RGB: return VK_FORMAT_R8G8B8_UNORM;
	case EPixelFormat::RGBA: return VK_FORMAT_R8G8B8A8_UNORM;
	case EPixelFormat::BGR: return VK_FORMAT_B8G8R8_UNORM;
	case EPixelFormat::BGRA: return VK_FORMAT_B8G8R8A8_UNORM;
	case EPixelFormat::RGB16: return VK_FORMAT_R16G16B16_SFLOAT;
	case EPixelFormat::RGB32: return VK_FORMAT_R32G32B32_SFLOAT;
	case EPixelFormat::RGBA16: return VK_FORMAT_R16G16B16A16_SFLOAT;
	case EPixelFormat::RGBA32: return VK_FORMAT_R32G32B32A32_SFLOAT;
	case EPixelFormat::D32: return VK_FORMAT_D32_SFLOAT;
	case EPixelFormat::D32S8: return VK_FORMAT_D32_SFLOAT_S8_UINT;
	case EPixelFormat::D24S8: return VK_FORMAT_D24_UNORM_S8_UINT;
	default: return VK_FORMAT_R8G8B8A8_UNORM;
	}
}

template<>
inline auto to_vk_enum(ESampleFilter e)
{
	switch (e)
	{
	case ESampleFilter::Point: return VK_FILTER_NEAREST;
	default: return VK_FILTER_LINEAR;
	}
}

template<>
inline auto to_vk_enum(ESampleMode e)
{
	switch (e)
	{
	case ESampleMode::RepeatMirror: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case ESampleMode::ClampToEdge: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case ESampleMode::ClampToBorder: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	default: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}
}