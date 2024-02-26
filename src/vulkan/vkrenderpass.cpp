#include "vkrenderpass.hpp"





Framebuffer::Framebuffer()
	: m_vkFramebuffer(VK_NULL_HANDLE)
{
}

Framebuffer::Framebuffer(Framebuffer&& framebuffer) noexcept
	: m_vkFramebuffer(framebuffer.m_vkFramebuffer)
{
	framebuffer.m_vkFramebuffer = VK_NULL_HANDLE;
}

Framebuffer::~Framebuffer()
{
	if (m_vkFramebuffer != VK_NULL_HANDLE)
	{
		vkDestroyFramebuffer(VkGlobals::vkDevice, m_vkFramebuffer, VkGlobals::vkAllocatorCallback);
		m_vkFramebuffer = VK_NULL_HANDLE;
	}
}

Framebuffer& Framebuffer::operator=(Framebuffer&& framebuffer) noexcept
{
	std::swap(m_vkFramebuffer, framebuffer.m_vkFramebuffer);
	return *this;
}





Renderpass::Renderpass()
	: m_vkRenderPass(VK_NULL_HANDLE)
	, m_colorCount(0)
	, m_hasDepth(false)
{
}

Renderpass::Renderpass(Renderpass&& renderpass) noexcept
	: m_vkRenderPass(renderpass.m_vkRenderPass)
	, m_colorCount(renderpass.m_colorCount)
	, m_hasDepth(renderpass.m_hasDepth)
{
	renderpass.m_vkRenderPass = VK_NULL_HANDLE;
	renderpass.m_colorCount = 0;
	renderpass.m_hasDepth = false;
}

Renderpass::~Renderpass()
{
	if (m_vkRenderPass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(VkGlobals::vkDevice, m_vkRenderPass, VkGlobals::vkAllocatorCallback);
		m_vkRenderPass = VK_NULL_HANDLE;
	}
}

Renderpass& Renderpass::operator=(Renderpass&& renderpass) noexcept
{
	std::swap(m_vkRenderPass, renderpass.m_vkRenderPass);
	std::swap(m_colorCount, renderpass.m_colorCount);
	std::swap(m_hasDepth, renderpass.m_hasDepth);
	return *this;
}

Framebuffer Renderpass::CreateFramebuffer(uint32_t width, uint32_t height, const std::vector<VkImageView>& views) const
{
	Framebuffer framebuffer;

	VkFramebufferCreateInfo framebufInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufInfo.renderPass = m_vkRenderPass;
	framebufInfo.width = width;
	framebufInfo.height = height;
	framebufInfo.layers = 1;
	framebufInfo.attachmentCount = uint32_t(views.size());
	framebufInfo.pAttachments = views.data();
	VK_ASSERT(vkCreateFramebuffer(VkGlobals::vkDevice, &framebufInfo, VkGlobals::vkAllocatorCallback, &framebuffer.m_vkFramebuffer));

	return framebuffer;
}



Renderpass RenderpassBuilder::Create()
{
	Renderpass renderpass;

	uint32_t colorAttachmentCount = uint32_t(m_AttachmentReferences.size()) - uint32_t(usedDepth);

	renderpass.m_hasDepth = usedDepth;
	renderpass.m_colorCount = colorAttachmentCount;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = colorAttachmentCount;
	subpass.pColorAttachments = m_AttachmentReferences.data();
	if (usedDepth)
	{
		subpass.pDepthStencilAttachment = &m_AttachmentReferences.back();
	}


	std::vector<VkSubpassDependency> dependencies(1);

	// In this subpass ...
	dependencies[0].dstSubpass = 0;
	// ... at this pipeline stage ...
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	// ... wait before performing these operations ...
	dependencies[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	// ... until all operations of this type ...
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
	// ... at these stages ...
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	// ... occuring in submission order prior to vkCmdBeginRenderPass ...
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	// ... have completed execution.

	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderpassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderpassInfo.attachmentCount = uint32_t(m_AttachmentDescriptions.size());
	renderpassInfo.pAttachments = m_AttachmentDescriptions.data();
	renderpassInfo.subpassCount = 1;
	renderpassInfo.pSubpasses = &subpass;
	renderpassInfo.dependencyCount = uint32_t(dependencies.size());
	renderpassInfo.pDependencies = dependencies.data();

	VK_ASSERT(vkCreateRenderPass(VkGlobals::vkDevice, &renderpassInfo, VkGlobals::vkAllocatorCallback, &renderpass.m_vkRenderPass));

	return renderpass;
}

RenderpassBuilder& RenderpassBuilder::AddAttachment(VkFormat format, VkImageLayout initialLayout, VkImageLayout finalLayout)
{
	VkAttachmentDescription attachment = {};
	attachment.format = format;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = initialLayout;
	attachment.finalLayout = finalLayout;

	VkAttachmentReference attachmentRef = {};
	attachmentRef.attachment = count;
	attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	m_AttachmentDescriptions.push_back(attachment);
	m_AttachmentReferences.push_back(attachmentRef);

	++count;
	return *this;
}

RenderpassBuilder& RenderpassBuilder::IsDepth()
{
	usedDepth = true;
	m_AttachmentReferences[count - 1].layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	return *this;
}

RenderpassBuilder& RenderpassBuilder::ClearOnBegin()
{
	m_AttachmentDescriptions[count - 1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	return *this;
}