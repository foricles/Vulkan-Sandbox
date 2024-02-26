workspace "VulkanSandbox"
	architecture "x64"
	configurations {"Release", "Debug"}

location "solution"

project "Sandbox"
	kind "WindowedApp"
	rtti "Off"
	language "C++"
	cppdialect "C++17"
	stringpooling "Off"
	floatingpoint "Fast"
	vectorextensions "AVX2"
	exceptionhandling "Off"

	targetdir "build/bin/%{cfg.buildcfg}"
	objdir "build/obj/%{cfg.buildcfg}"


	files
	{
		"src/**.c",
		"src/**.h",
		"src/**.cpp",
		"src/**.hpp",
		"shaders/**.almfx"
	}

	includedirs --directories
	{
		"thirdparty/include/"
	}

	libdirs 
	{ 
		"thirdparty/lib/",
	}

	links
	{
		"vulkan-1",
		"dxcompiler.lib",
	}

	defines
	{
		"_USE_MATH_DEFINES",
		"VK_USE_PLATFORM_WIN32_KHR",
		"SPIRV_REFLECT_USE_SYSTEM_SPIRV_H",
		"_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING",
		"NOMINMAX"
	}

filter "configurations:Debug"
	defines {}
	symbols "On"
	debugdir "%{cfg.targetdir}"
	links
	{
		"assimp_db.lib",
		"zlib_db.lib",
	}
	
filter "configurations:Release"
	defines {}
	optimize "On"
	debugdir "%{cfg.targetdir}"
	links
	{
		"assimp_rel.lib",
		"zlib_rel.lib",
	}
filter{}

postbuildcommands ("{COPYDIR} ../shaders/ %{cfg.targetdir}/shaders")
postbuildcommands ("{COPYDIR} ../models/ %{cfg.targetdir}/models")