#pragma once
#include <array>
#define STRINGIFY(x) L#x
#define TOSTRING(x) STRINGIFY(x)
#define DECLARE_DESC_SET_OFFSET(name, offset) constexpr int name = offset; constexpr auto name##_str = STRINGIFY(offset)

DECLARE_DESC_SET_OFFSET(BUFFER_DS_OFFSET, 0);
DECLARE_DESC_SET_OFFSET(SAMPLER_DS_OFFSET, 128);
DECLARE_DESC_SET_OFFSET(SRV_DS_OFFSET, 256);
DECLARE_DESC_SET_OFFSET(UAV_DS_OFFSET, 384);


enum class EShaderType
{
	Vertex,
	Fragment,
	Compute,
	RayGeneration,
	RayMiss,
	RayClosestHit,

	COUNT
};

enum class EDescriptorType
{
	Sampler = 0,
	SampledImage = 2,
	StorageImage = 3,
	UniformBuffer = 6,
	StorageBuffer = 7,
	InputAttachment = 10,
	AccelerationStructure = 1000150000
};

enum class EInputFormat
{
	Uint,
	Uint3,
	Uint3x3,
	Float,
	Float2,
	Float3,
	Float4,
	Float2x2,
	Float3x3,
	Float4x4,

	COUNT
};

enum class EBufferType
{
	Vertex,
	Index,
	Uniform,
	Storage,

	COUNT
};

enum class ETopology
{
	Point,
	Line,
	Triangle,

	COUNT
};

enum class EFillMode
{
	Fill,
	Line,
	Point,

	COUNT
};

enum class ECull
{
	None,
	Front,
	Back,
	FrontAndBack,

	COUNT
};

enum class EFrontFace
{
	CW,
	CCW,

	COUNT
};

enum class EDepthFunc
{
	Never,
	Less,
	Equal,
	LessEqual,
	Greater,
	NotEqual,
	GreaterEqual,
	Allways,

	COUNT
};

enum class EBlendFactor
{
	Zero,
	One,
	SrcColor,
	OneMinusSrcColor,
	DstColor,
	OneMinusDstColor,
	SrcAlpha,
	OneMinusSrcAlpha,
	DstAlpha,
	OneMinus,

	COUNT
};

enum struct EBlenType
{
	COUNT
};

struct Vertex
{
	float px{ 0 }, py{ 0 }, pz{ 0 };
	float nx{ 0 }, ny{ 0 }, nz{ 0 };
	float tx{ 0 }, ty{ 0 }, tz{ 0 };
	float bx{ 0 }, by{ 0 }, bz{ 0 };
	float uv{ 0 }, uw{ 0 };
};

enum class EPixelFormat
{
	Mono,
	RGB,
	RGBA,
	BGR,
	BGRA,
	RGB16,
	RGB32,
	RGBA16,
	RGBA32,

	D32,
	D32S8,
	D24S8,

	COUNT
};

enum class ESampleFilter
{
	Point,
	Linear
};

enum class ESampleMode
{
	Repeat,
	RepeatMirror,
	ClampToEdge,
	ClampToBorder
};