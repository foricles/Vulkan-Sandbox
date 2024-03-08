#include "app.hpp"
#include <random>
#include "../vulkan/vkcommon.hpp"
#include "../vulkan/vkengine.hpp"
#include "../vulkan/vkutils.hpp"
#include "../vulkan/vkrenderpass.hpp"
#include "../vulkan/shader/graphicshader.hpp"
#include "../vulkan/shader/raytraceshader.hpp"
#include "../vulkan/vktexture.hpp"
#include "../vulkan/vkbuffer.hpp"
#include "../vulkan/vkacstructure.hpp"
#include "modelloader.hpp"
#include "camera.hpp"

#include "helper.hpp"

#define SSAO_KERNEL 24

enum EShaderFlags
{
	esf_HasDiffuseMap = BIT(0),
	esf_HasNormalMap = BIT(1)
};

enum EShaderSSAOFlags
{
	essf_SSAO = 0,
	essf_Blur = BIT(0)
};

struct ConstantBuffer
{
	math::mat4 view;
	math::mat4 proj;
	math::mat4 view_invert;
	math::mat4 proj_invert;
	math::vec4 directionLight;
	math::vec4 screenSize;
	math::vec4 frustum;
	math::vec4 hdrTonemap;
};

struct GpuMaterial
{
	Texture diffuse;
	Texture normal;
	uint64_t flags{ 0 };
};

struct GpuMesh
{
	Buffer indices{ EBufferType::Index };
	Buffer vertices{ EBufferType::Vertex };
	uint32_t materialId{ 0 };
	uint32_t indexCount{ 0 };
};

struct GBuffer
{
	Texture diffuse;
	Texture normal;
	Texture txrGlobalPosition;
	Renderpass renderpass;
	Framebuffer framebuffer;
};

struct SSAO
{
	Texture txrSSAO;
	Renderpass renderpass;
	Framebuffer framebuffer;
	Buffer kernel{ EBufferType::Storage };
};

struct DirectShadow
{
	Texture txrShadowMask;
	ShaderRaytrace shaderShadows;
	AccelerationStructure topAccStructure;
	AccelerationStructure bottomAccStructure;
};

struct AppPimpl
{
	uint32_t swapchainImage{ 0 };
	uint64_t swapchainGeneration{ 0 };
	VkSemaphore acquireImageSem{ VK_NULL_HANDLE };
	VkSemaphore presentImageSem{ VK_NULL_HANDLE };
	VkCommandBuffer commandBufer{ VK_NULL_HANDLE };

	GBuffer gbuffer;
	SSAO ssao;

	Renderpass lightingRenderpass;
	Renderpass finalizeRenderpass;

	Framebuffer hdrTargetFramebuffer;
	Framebuffer finalizeFramebuffers[VulkanEngine::kSwapchainImageCount];

	Sampler linearSampler;
	Sampler pointSampler;

	ConstantBuffer constants;
	Buffer constantBuffer{ EBufferType::Uniform, true };
	Buffer fullScreenIndecies{ EBufferType::Index };
	RenderState fullscreenState;

	Texture txrDepth;
	Texture txrHdrTarget;

	ShaderGraphics shaderGBuffer;
	ShaderGraphics shaderLighting;
	ShaderGraphics shaderSSAO;
	ShaderGraphics shaderHDRTonemap;

	VkRect2D rndArea{};
	VkViewport viewport{};
	VkRect2D scissor{};

	DirectShadow directionalShadow;
	Camera mainCamera;

	std::vector<GpuMesh> meshesToDraw;
	std::unordered_map<uint32_t, GpuMaterial> materials;
};




void App::Init()
{
	m_pApp = new AppPimpl();

	m_pApp->fullscreenState.cullMode = ECull::None;
	m_pApp->fullscreenState.hasInputAttachment = false;

	m_pApp->acquireImageSem = VulkanEngine::CreateVkSemaphore();
	m_pApp->presentImageSem = VulkanEngine::CreateVkSemaphore();
	m_pApp->commandBufer = VulkanEngine::CreateCommandBuffer();


	m_pApp->linearSampler.Create(ESampleFilter::Linear, ESampleMode::Repeat, 16, 4);
	m_pApp->pointSampler.Create(ESampleFilter::Point, ESampleMode::Repeat, 0, 1);


	{
		auto data = helpers::sb_read_file("shaders\\gbuffer.almfx");
		m_pApp->shaderGBuffer.SetSource(reinterpret_cast<char*>(data.data()));
		m_pApp->shaderGBuffer.MarkProgram(EShaderType::Vertex, "MainVS");
		m_pApp->shaderGBuffer.MarkProgram(EShaderType::Fragment, "MainPS");
	}
	{
		auto data = helpers::sb_read_file("shaders\\lighting.almfx");
		m_pApp->shaderLighting.SetSource(reinterpret_cast<char*>(data.data()));
		m_pApp->shaderLighting.MarkProgram(EShaderType::Vertex, "MainVS");
		m_pApp->shaderLighting.MarkProgram(EShaderType::Fragment, "MainPS");
	}
	{
		auto data = helpers::sb_read_file("shaders\\hdrtonemap.almfx");
		m_pApp->shaderHDRTonemap.SetSource(reinterpret_cast<char*>(data.data()));
		m_pApp->shaderHDRTonemap.MarkProgram(EShaderType::Vertex, "MainVS");
		m_pApp->shaderHDRTonemap.MarkProgram(EShaderType::Fragment, "MainPS");
	}
	{
		auto data = helpers::sb_read_file("shaders\\ssao.almfx");
		m_pApp->shaderSSAO.SetSource(reinterpret_cast<char*>(data.data()));
		m_pApp->shaderSSAO.MarkProgram(EShaderType::Vertex, "MainVS");
		m_pApp->shaderSSAO.MarkProgram(EShaderType::Fragment, "MainPS");
	}
	{
		auto data = helpers::sb_read_file("shaders\\shadowsraytrace.almfx");
		m_pApp->directionalShadow.shaderShadows.SetSource(reinterpret_cast<char*>(data.data()));
		m_pApp->directionalShadow.shaderShadows.MarkProgram(EShaderType::RayGeneration, "RayGenerationRS");
		m_pApp->directionalShadow.shaderShadows.MarkProgram(EShaderType::RayClosestHit, "CloseHitRS");
		m_pApp->directionalShadow.shaderShadows.MarkProgram(EShaderType::RayMiss, "MissRS");
		m_pApp->directionalShadow.shaderShadows.SetState(0, 0);
	}

	m_pApp->fullScreenIndecies.Load(std::vector<uint32_t>{0, 1, 2, 1, 3, 2});

	m_pApp->constants.directionLight = math::vec4(1, -1, 0, 1);
	m_pApp->constants.view = math::mat4(1);
	m_pApp->constants.hdrTonemap.x = 2.2f;
	m_pApp->constants.hdrTonemap.y = 0.3f;

	m_pApp->constantBuffer.Load(&m_pApp->constants, sizeof(ConstantBuffer));


	Model diorama = ModelLoader().Load("models\\diorama\\diorama_ww2\\diorama.fbx");
	//Model diorama = ModelLoader().Load("models\\backpack\\backpack.fbx");
	for (auto& material : diorama.materials)
	{
		GpuMaterial& gpuMaterial = m_pApp->materials[material.first];
		if (material.second.diffuseTexture.IsValid())
		{
			RawTexture& diffuse = material.second.diffuseTexture;
			gpuMaterial.diffuse.Load(diffuse.data, diffuse.width, diffuse.height, EPixelFormat::RGBA, 4);
			gpuMaterial.flags |= esf_HasDiffuseMap;
		}
		if (material.second.normalTexture.IsValid())
		{
			RawTexture& normal = material.second.normalTexture;
			gpuMaterial.normal.Load(normal.data, normal.width, normal.height, EPixelFormat::RGBA, 4);
			gpuMaterial.flags |= esf_HasNormalMap;
		}
	}

	m_pApp->meshesToDraw.resize(diorama.meshes.size());
	for (size_t i(0); i < diorama.meshes.size(); ++i)
	{
		Mesh& mesh = diorama.meshes[i];
		GpuMesh& gpuMesh = m_pApp->meshesToDraw[i];

		gpuMesh.materialId = mesh.materialId;
		gpuMesh.indexCount = uint32_t(mesh.indices.size());
		gpuMesh.indices.Load(mesh.indices);
		gpuMesh.vertices.Load(mesh.vertices);
	}

	std::sort(m_pApp->meshesToDraw.begin(), m_pApp->meshesToDraw.end(), [](const GpuMesh& meshA, const GpuMesh& meshB) {
		return meshA.materialId > meshB.materialId;
	});


	AccStructBuilder builder = m_pApp->directionalShadow.bottomAccStructure.Builder(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);
	for (auto& mesh : m_pApp->meshesToDraw)
	{
		builder.AddTriangles(mesh.vertices, mesh.indices)
			.MaxVertices(mesh.vertices.size() / sizeof(Vertex))
			.Primitives(mesh.indexCount / 3)
			.Stride(sizeof(Vertex));
	}
	builder.Build();


	m_pApp->directionalShadow.topAccStructure.Builder(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR)
		.AddAccelerationStructure(&m_pApp->directionalShadow.bottomAccStructure)
		.Build();


	std::default_random_engine generator;
	std::uniform_real_distribution<float> rnd_floats(0, 1);

	std::vector<math::vec4> ssaoKernel(SSAO_KERNEL);
	for (math::vec4& dir : ssaoKernel)
	{
		math::vec3 point;
		point.x = rnd_floats(generator) * 2.0f - 1.0f;
		point.y = rnd_floats(generator);
		point.z = rnd_floats(generator) * 2.0f - 1.0f;

		point = point.normalized() * (rnd_floats(generator) + 0.1f);

		dir.x = point.x;
		dir.y = point.y;
		dir.z = point.z;
	}

	m_pApp->ssao.kernel.Load(ssaoKernel);

	m_pApp->mainCamera.Position().y = 300;
}

void App::Shutdown()
{
	VulkanEngine::DestroyCommandBuffer(m_pApp->commandBufer);
	VulkanEngine::DestroyVkSemaphore(m_pApp->presentImageSem);
	VulkanEngine::DestroyVkSemaphore(m_pApp->acquireImageSem);

	delete m_pApp;
}

void App::OnWidowResize(uint32_t width, uint32_t height)
{
	m_pApp->rndArea.offset = { 0, 0 };
	m_pApp->rndArea.extent.width = VkGlobals::swapchain.width;
	m_pApp->rndArea.extent.height = VkGlobals::swapchain.height;

	m_pApp->viewport.x = 0.0f;
	m_pApp->viewport.y = 0.0f;
	m_pApp->viewport.width = float(VkGlobals::swapchain.width);
	m_pApp->viewport.height = float(VkGlobals::swapchain.height);
	m_pApp->viewport.minDepth = 0.0f;
	m_pApp->viewport.maxDepth = 1.0f;

	m_pApp->scissor.offset = { 0, 0 };
	m_pApp->scissor.extent = { VkGlobals::swapchain.width , VkGlobals::swapchain.height };

	m_pApp->gbuffer.diffuse.Create(
		VkGlobals::swapchain.width,
		VkGlobals::swapchain.height,
		EPixelFormat::RGBA
	);

	m_pApp->gbuffer.normal.Create(
		VkGlobals::swapchain.width,
		VkGlobals::swapchain.height,
		EPixelFormat::RGBA
	);
	m_pApp->gbuffer.txrGlobalPosition.Create(
		VkGlobals::swapchain.width,
		VkGlobals::swapchain.height,
		EPixelFormat::RGBA32
	);

	m_pApp->txrHdrTarget.Create(
		VkGlobals::swapchain.width,
		VkGlobals::swapchain.height,
		EPixelFormat::RGBA16
	);

	m_pApp->ssao.txrSSAO.Create(
		VkGlobals::swapchain.width,
		VkGlobals::swapchain.height,
		EPixelFormat::Mono
	);

	m_pApp->txrDepth.Create(
		VkGlobals::swapchain.width,
		VkGlobals::swapchain.height,
		EPixelFormat::D32
	);

	m_pApp->directionalShadow.txrShadowMask.Create(
		VkGlobals::swapchain.width,
		VkGlobals::swapchain.height,
		EPixelFormat::Mono
	);

	if (!m_isRenderInit)
	{
		m_isRenderInit = true;

		m_pApp->gbuffer.renderpass = RenderpassBuilder()
			.AddAttachment(EPixelFormat::RGBA, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				.ClearOnBegin()
			.AddAttachment(EPixelFormat::RGBA, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				.ClearOnBegin()
			.AddAttachment(EPixelFormat::RGBA32, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				.ClearOnBegin()
			.AddAttachment(EPixelFormat::D32, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL)
				.ClearOnBegin()
				.IsDepth()
			.Create();

		m_pApp->lightingRenderpass = RenderpassBuilder()
			.AddAttachment(EPixelFormat::RGBA16, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			.Create();

		m_pApp->ssao.renderpass = RenderpassBuilder()
			.AddAttachment(EPixelFormat::Mono, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				.ClearOnBegin()
			.Create();

		m_pApp->finalizeRenderpass = RenderpassBuilder()
			.AddAttachment(VkGlobals::swapchain.format.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
			.Create();

		for (auto& gpuMaterial : m_pApp->materials)
		{
			m_pApp->shaderGBuffer.SetState(m_pApp->gbuffer.renderpass, gpuMaterial.second.flags, gpuMaterial.first, RenderState());
			if (m_pApp->shaderGBuffer.HasBindables())
			{
				bool hasTexture = false;
				ShaderBinder binder = m_pApp->shaderGBuffer.Binder();
				if (gpuMaterial.second.flags & esf_HasDiffuseMap)
				{
					hasTexture = true;
					binder.Image(gpuMaterial.second.diffuse, 0);
				}
				if (gpuMaterial.second.flags & esf_HasNormalMap)
				{
					hasTexture = true;
					binder.Image(gpuMaterial.second.normal, 1);
				}
				if (hasTexture)
				{
					binder.ImageSampler(m_pApp->linearSampler, 0);
				}
				binder.Bind();
			}
		}
	}

	m_pApp->gbuffer.framebuffer = m_pApp->gbuffer.renderpass.CreateFramebuffer(
		VkGlobals::swapchain.width,
		VkGlobals::swapchain.height,
		{
			m_pApp->gbuffer.diffuse.GetView(),
			m_pApp->gbuffer.normal.GetView(),
			m_pApp->gbuffer.txrGlobalPosition.GetView(),
			m_pApp->txrDepth.GetView()
		}
	);

	m_pApp->hdrTargetFramebuffer = m_pApp->lightingRenderpass.CreateFramebuffer(
		VkGlobals::swapchain.width,
		VkGlobals::swapchain.height,
		{ m_pApp->txrHdrTarget.GetView() }
	);

	m_pApp->ssao.framebuffer = m_pApp->ssao.renderpass.CreateFramebuffer(
		VkGlobals::swapchain.width,
		VkGlobals::swapchain.height,
		{ m_pApp->ssao.txrSSAO.GetView() }
	);

	for (uint32_t i(0); i < VulkanEngine::kSwapchainImageCount; ++i)
	{
		m_pApp->finalizeFramebuffers[i] = m_pApp->finalizeRenderpass.CreateFramebuffer(
			VkGlobals::swapchain.width,
			VkGlobals::swapchain.height,
			{ VkGlobals::swapchain.views[i] }
		);
	}

	const float nearPlane = 0.3f;
	const float farPlane = 3000.f;

	m_pApp->mainCamera.PerspectiveProjection(120, VkGlobals::swapchain.width, VkGlobals::swapchain.height, nearPlane, farPlane);

	m_pApp->constants.proj = m_pApp->mainCamera.Projection();
	m_pApp->constants.proj_invert = m_pApp->constants.proj.inverted();
	m_pApp->constants.screenSize.x = float(VkGlobals::swapchain.width);
	m_pApp->constants.screenSize.y = float(VkGlobals::swapchain.height);
	m_pApp->constants.screenSize.z = 1.f / m_pApp->constants.screenSize.x;
	m_pApp->constants.screenSize.w = 1.f / m_pApp->constants.screenSize.y;
	m_pApp->constants.frustum.x = nearPlane;
	m_pApp->constants.frustum.y = farPlane;

	Shader::PerFrameDescriptors::Binder().UniformBuffer(m_pApp->constantBuffer, 0).Bind();


	m_pApp->shaderLighting.SetState(m_pApp->lightingRenderpass, 0, 0, m_pApp->fullscreenState);
	m_pApp->shaderLighting.Binder()
			.ImageSampler(m_pApp->pointSampler, 0)
			.Image(m_pApp->gbuffer.diffuse, 0)
			.Image(m_pApp->gbuffer.normal, 1)
			.Image(m_pApp->ssao.txrSSAO, 2)
			.Image(m_pApp->txrDepth, 3, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL)
			.Image(m_pApp->directionalShadow.txrShadowMask, 4)
		.Bind();


	m_pApp->shaderHDRTonemap.SetState(m_pApp->finalizeRenderpass, 0, 0, m_pApp->fullscreenState);
	m_pApp->shaderHDRTonemap.Binder()
			.Image(m_pApp->txrHdrTarget, 0)
			.ImageSampler(m_pApp->pointSampler, 0)
		.Bind();


	m_pApp->shaderSSAO.SetState(m_pApp->ssao.renderpass, 0, 0, m_pApp->fullscreenState);
	m_pApp->shaderSSAO.Binder()
			.ImageSampler(m_pApp->pointSampler, 0)
			.Image(m_pApp->gbuffer.normal, 0)
			.Image(m_pApp->txrDepth, 1, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL)
			.StorageBufferReadonly(m_pApp->ssao.kernel, 2)
		.Bind();


	m_pApp->directionalShadow.shaderShadows.SetState(0, 0);
	m_pApp->directionalShadow.shaderShadows.Binder()
			.StorageImage(m_pApp->directionalShadow.txrShadowMask, 0)
			.Image(m_pApp->gbuffer.txrGlobalPosition, 1)
			.AccelerationStructure(m_pApp->directionalShadow.topAccStructure.Get(), 0)
		.Bind();
}

void App::Render()
{
	if (m_pApp->swapchainGeneration != VkGlobals::swapchain.generation)
	{
		m_pApp->swapchainGeneration = VkGlobals::swapchain.generation;
		return;
	}

	VkResult result = vkAcquireNextImageKHR(VkGlobals::vkDevice, VkGlobals::swapchain.vkSwapchain, UINT64_MAX, m_pApp->acquireImageSem, VK_NULL_HANDLE, &m_pApp->swapchainImage);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		return;
	}


	m_pApp->constants.view = m_pApp->mainCamera.View();
	m_pApp->constants.view_invert = m_pApp->constants.view.inverted();
	m_pApp->constantBuffer.Load(&m_pApp->constants, sizeof(ConstantBuffer));

	VK_ASSERT(vkResetCommandBuffer(m_pApp->commandBufer, 0));


	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VK_ASSERT(vkBeginCommandBuffer(m_pApp->commandBufer, &beginInfo));

	vkCmdResetQueryPool(m_pApp->commandBufer, VkGlobals::vkQueryPool, 0, VulkanEngine::kQueryCount);
	vkCmdWriteTimestamp(m_pApp->commandBufer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VkGlobals::vkQueryPool, 0);

	GBufferPass();

	SSAOPass();

	RaytraceShadows();

	LightingPass();

	FinalHDRPass();

	vkCmdWriteTimestamp(m_pApp->commandBufer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VkGlobals::vkQueryPool, 1);

	VK_ASSERT(vkEndCommandBuffer(m_pApp->commandBufer));


	VkPipelineStageFlags stageMask = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &m_pApp->commandBufer;
	submit.waitSemaphoreCount = 1;
	submit.signalSemaphoreCount = 1;
	submit.pWaitSemaphores = &m_pApp->acquireImageSem;
	submit.pSignalSemaphores = &m_pApp->presentImageSem;
	submit.pWaitDstStageMask = &stageMask;
	VK_ASSERT(vkQueueSubmit(VkGlobals::queue.vkQueue, 1, &submit, VK_NULL_HANDLE));


	VkPresentInfoKHR pPresentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	pPresentInfo.waitSemaphoreCount = 1;
	pPresentInfo.pWaitSemaphores = &m_pApp->presentImageSem;
	pPresentInfo.swapchainCount = 1;
	pPresentInfo.pSwapchains = &VkGlobals::swapchain.vkSwapchain;
	pPresentInfo.pImageIndices = &m_pApp->swapchainImage;

	vkQueuePresentKHR(VkGlobals::queue.vkQueue, &pPresentInfo);
	vkDeviceWaitIdle(VkGlobals::vkDevice);

	std::array<uint64_t, VulkanEngine::kQueryCount> results{ 0 };
	vkGetQueryPoolResults(
		VkGlobals::vkDevice,
		VkGlobals::vkQueryPool,
		0, 2,
		results.size() * sizeof(uint64_t),
		results.data(),
		sizeof(uint64_t),
		VK_QUERY_RESULT_64_BIT
	);

	m_gpuTime = (double(results[1]) - double(results[0])) * VulkanEngine::GetGpuTimestampPeriod() * 1e-6;
}

void App::GBufferPass()
{
	std::vector<VkClearValue> clearValues(4);
	clearValues[0].color = { {0.3, 0.3, 0.3, 1} };
	clearValues[1].color = { {0.5, 0.5, 0.5, 1} };
	clearValues[2].color = { {0, 0, 0, 1} };
	clearValues[3].depthStencil = { 1, 0 };

	VkRenderPassBeginInfo renderPassBegin = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassBegin.renderPass = m_pApp->gbuffer.renderpass;
	renderPassBegin.framebuffer = m_pApp->gbuffer.framebuffer;
	renderPassBegin.renderArea = m_pApp->rndArea;
	renderPassBegin.clearValueCount = uint32_t(clearValues.size());
	renderPassBegin.pClearValues = clearValues.data();
	vkCmdBeginRenderPass(m_pApp->commandBufer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdSetViewport(m_pApp->commandBufer, 0, 1, &m_pApp->viewport);
	vkCmdSetScissor(m_pApp->commandBufer, 0, 1, &m_pApp->scissor);

	const VkDeviceSize offsets[] = { 0 };
	uint32_t currentMaterialID = UINT32_MAX;
	for (auto& gpuMesh : m_pApp->meshesToDraw)
	{
		if (currentMaterialID != gpuMesh.materialId)
		{
			currentMaterialID = gpuMesh.materialId;
			m_pApp->shaderGBuffer.SetState(m_pApp->gbuffer.renderpass, m_pApp->materials[gpuMesh.materialId].flags, gpuMesh.materialId, RenderState());
			m_pApp->shaderGBuffer.Bind(m_pApp->commandBufer);
		}

		VkBuffer b[] = { gpuMesh.vertices };
		vkCmdBindVertexBuffers(m_pApp->commandBufer, 0, 1, b, offsets);
		vkCmdBindIndexBuffer(m_pApp->commandBufer, gpuMesh.indices, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(m_pApp->commandBufer, gpuMesh.indexCount, 1, 0, 0, 0);
	}


	vkCmdEndRenderPass(m_pApp->commandBufer);
}

void App::LightingPass()
{
	VkImageMemoryBarrier2 imgBarier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
	imgBarier.srcStageMask = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
	imgBarier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
	imgBarier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	imgBarier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
	imgBarier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	imgBarier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imgBarier.image = m_pApp->directionalShadow.txrShadowMask.Get();
	imgBarier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imgBarier.subresourceRange.layerCount = 1;
	imgBarier.subresourceRange.levelCount = 1;
	imgBarier.subresourceRange.baseArrayLayer = 0;
	imgBarier.subresourceRange.baseMipLevel = 0;

	VkDependencyInfo depInfo = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	depInfo.imageMemoryBarrierCount = 1;
	depInfo.pImageMemoryBarriers = &imgBarier;
	vkCmdPipelineBarrier2(m_pApp->commandBufer, &depInfo);

	m_pApp->shaderLighting.SetState(m_pApp->lightingRenderpass, 0, 0, m_pApp->fullscreenState);

	VkRenderPassBeginInfo renderPassBegin = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassBegin.renderPass = m_pApp->lightingRenderpass;
	renderPassBegin.framebuffer = m_pApp->hdrTargetFramebuffer;
	renderPassBegin.renderArea = m_pApp->rndArea;
	vkCmdBeginRenderPass(m_pApp->commandBufer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdSetViewport(m_pApp->commandBufer, 0, 1, &m_pApp->viewport);
	vkCmdSetScissor(m_pApp->commandBufer, 0, 1, &m_pApp->scissor);

	m_pApp->shaderLighting.Bind(m_pApp->commandBufer);

	vkCmdBindIndexBuffer(m_pApp->commandBufer, m_pApp->fullScreenIndecies, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(m_pApp->commandBufer, 6, 1, 0, 0, 0);

	vkCmdEndRenderPass(m_pApp->commandBufer);
}

void App::SSAOPass()
{
	m_pApp->shaderSSAO.SetState(m_pApp->ssao.renderpass, 0, 0, m_pApp->fullscreenState);

	std::vector<VkClearValue> clearValues(1);
	clearValues[0].color = { {1, 1, 1, 1} };

	VkRenderPassBeginInfo renderPassBegin = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassBegin.renderPass = m_pApp->ssao.renderpass;
	renderPassBegin.framebuffer = m_pApp->ssao.framebuffer;
	renderPassBegin.renderArea = m_pApp->rndArea;
	renderPassBegin.clearValueCount = uint32_t(clearValues.size());
	renderPassBegin.pClearValues = clearValues.data();
	vkCmdBeginRenderPass(m_pApp->commandBufer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdSetViewport(m_pApp->commandBufer, 0, 1, &m_pApp->viewport);
	vkCmdSetScissor(m_pApp->commandBufer, 0, 1, &m_pApp->scissor);

	m_pApp->shaderSSAO.Bind(m_pApp->commandBufer);
	vkCmdBindIndexBuffer(m_pApp->commandBufer, m_pApp->fullScreenIndecies, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(m_pApp->commandBufer, 6, 1, 0, 0, 0);

	vkCmdEndRenderPass(m_pApp->commandBufer);
}

void App::FinalHDRPass()
{
	m_pApp->shaderHDRTonemap.SetState(m_pApp->finalizeRenderpass, 0, 0, m_pApp->fullscreenState);


	VkRenderPassBeginInfo renderPassBegin = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassBegin.renderPass = m_pApp->finalizeRenderpass;
	renderPassBegin.framebuffer = m_pApp->finalizeFramebuffers[m_pApp->swapchainImage];
	renderPassBegin.renderArea = m_pApp->rndArea;
	vkCmdBeginRenderPass(m_pApp->commandBufer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdSetViewport(m_pApp->commandBufer, 0, 1, &m_pApp->viewport);
	vkCmdSetScissor(m_pApp->commandBufer, 0, 1, &m_pApp->scissor);

	m_pApp->shaderHDRTonemap.Bind(m_pApp->commandBufer);

	vkCmdBindIndexBuffer(m_pApp->commandBufer, m_pApp->fullScreenIndecies, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(m_pApp->commandBufer, 6, 1, 0, 0, 0);

	vkCmdEndRenderPass(m_pApp->commandBufer);
}

void App::RaytraceShadows()
{
	m_pApp->directionalShadow.shaderShadows.SetState(0, 0);



	static bool isInitialized = false;
	if (!isInitialized)
	{
		isInitialized = true;

		VkImageMemoryBarrier2 imgBarier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
		imgBarier.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		imgBarier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
		imgBarier.dstStageMask = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
		imgBarier.dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
		imgBarier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imgBarier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		imgBarier.image = m_pApp->directionalShadow.txrShadowMask.Get();
		imgBarier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgBarier.subresourceRange.layerCount = 1;
		imgBarier.subresourceRange.levelCount = 1;
		imgBarier.subresourceRange.baseArrayLayer = 0;
		imgBarier.subresourceRange.baseMipLevel = 0;

		VkDependencyInfo depInfo = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
		depInfo.imageMemoryBarrierCount = 1;
		depInfo.pImageMemoryBarriers = &imgBarier;
		vkCmdPipelineBarrier2(m_pApp->commandBufer, &depInfo);
		return;
	}

	VkImageMemoryBarrier2 imgBarier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
	imgBarier.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	imgBarier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
	imgBarier.dstStageMask = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
	imgBarier.dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
	imgBarier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imgBarier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	imgBarier.image = m_pApp->directionalShadow.txrShadowMask.Get();
	imgBarier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imgBarier.subresourceRange.layerCount = 1;
	imgBarier.subresourceRange.levelCount = 1;
	imgBarier.subresourceRange.baseArrayLayer = 0;
	imgBarier.subresourceRange.baseMipLevel = 0;

	VkDependencyInfo depInfo = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	depInfo.imageMemoryBarrierCount = 1;
	depInfo.pImageMemoryBarriers = &imgBarier;
	vkCmdPipelineBarrier2(m_pApp->commandBufer, &depInfo);


	m_pApp->directionalShadow.shaderShadows.Bind(m_pApp->commandBufer);
	m_pApp->directionalShadow.shaderShadows.Draw(m_pApp->commandBufer, VkGlobals::swapchain.width, VkGlobals::swapchain.height);
}


App::App()
	: m_pApp(nullptr)
	, m_isRenderInit(false)
	, m_gpuTime(0)
{

}

App::~App()
{

}


void App::Update(float dt)
{
	float speed = 50 * dt;
	if (Input.KeyPressed(EKeys::SHIFT))
	{
		speed *= 10;
	}
	if (Input.KeyPressed(EKeys::W))
	{
		m_pApp->mainCamera.Translate({ 0, 0, -speed });
	}
	if (Input.KeyPressed(EKeys::S))
	{
		m_pApp->mainCamera.Translate({ 0, 0, speed });
	}
	if (Input.KeyPressed(EKeys::A))
	{
		m_pApp->mainCamera.Translate({ -speed, 0, 0 });
	}
	if (Input.KeyPressed(EKeys::D))
	{
		m_pApp->mainCamera.Translate({ speed, 0, 0 });
	}
	if (Input.KeyPressed(EKeys::Q))
	{
		m_pApp->mainCamera.Translate({ 0, -speed, 0 });
	}
	if (Input.KeyPressed(EKeys::E))
	{
		m_pApp->mainCamera.Translate({ 0, speed, 0 });
	}

	static float dx = 0;
	static float dy = 0;
	if (Input.KeyPressed(EKeys::I))
	{
		dy += dt;
	}
	if (Input.KeyPressed(EKeys::K))
	{
		dy -= dt;
	}
	if (Input.KeyPressed(EKeys::J))
	{
		dx -= dt;
	}
	if (Input.KeyPressed(EKeys::L))
	{
		dx += dt;
	}
	m_pApp->mainCamera.SetRotation({ -dy, -dx, 0 });



	if (Input.KeyPressed(EKeys::F1))
	{
		m_pApp->constants.hdrTonemap.x += dt;
	}
	if (Input.KeyPressed(EKeys::F2))
	{
		m_pApp->constants.hdrTonemap.x -= dt;
	}

	if (Input.KeyPressed(EKeys::F3))
	{
		m_pApp->constants.hdrTonemap.y += dt;
	}
	if (Input.KeyPressed(EKeys::F4))
	{
		m_pApp->constants.hdrTonemap.y -= dt;
	}
}