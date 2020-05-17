// Helper code and boilerplate for Vulkan setup

#include <algorithm>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <array>

#include "vulkan.hpp"
#include "util.h"


// Pick a queue family to use
static std::optional<uint32_t> pickQueueFamily(std::vector<vk::QueueFamilyProperties> &queueFamilies) {
	// Pick any queue family that supports graphics
	for (size_t i = 0; i < queueFamilies.size(); i++) {
		if (queueFamilies.at(i).queueFlags & vk::QueueFlagBits::eGraphics)
			return i;
	}
	return {};
}


// Pick surface format with preference for a given format
static vk::SurfaceFormatKHR pickFormat(std::vector<vk::SurfaceFormatKHR> &formats, vk::SurfaceFormatKHR preferredFormat) {
	for (auto &format: formats) {
		if (format == preferredFormat) {
			return format;
		}
	}
	// Fall back to the first listed format
	return formats.at(0);
}


// Initial setup, create device
void VulkanState::init() {
	// Create instance, with extensions needed by GLFW
	uint32_t extension_count;
	const char **extensions = glfwGetRequiredInstanceExtensions(&extension_count);
	vk::InstanceCreateInfo instanceInfo{};
	instanceInfo.enabledExtensionCount = extension_count;
	instanceInfo.ppEnabledExtensionNames = extensions;
	instance = vk::createInstanceUnique(instanceInfo);

	auto physicalDevices = instance->enumeratePhysicalDevices();
	// todo: Just picking the first device for now...
	physicalDevice = physicalDevices.at(0);

	auto queueFamilies = physicalDevice.getQueueFamilyProperties();
	queueFamily = pickQueueFamily(queueFamilies).value();

	float queuePriority = 0.5;
	vk::DeviceQueueCreateInfo queueInfo{};
	queueInfo.queueFamilyIndex = queueFamily;
	queueInfo.queueCount = 1;
	queueInfo.pQueuePriorities = &queuePriority;

	vk::PhysicalDeviceFeatures enabledFeatures{};
	// Need large points for our particles
	enabledFeatures.largePoints = true;

	// todo: should verify extension support with physical device
	std::vector<const char*> requiredExtensions {
		"VK_KHR_swapchain",
	};

	vk::DeviceCreateInfo deviceInfo{};
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.pQueueCreateInfos = &queueInfo;
	deviceInfo.pEnabledFeatures = &enabledFeatures;
	deviceInfo.enabledExtensionCount = requiredExtensions.size();
	deviceInfo.ppEnabledExtensionNames = requiredExtensions.data();
	device = physicalDevice.createDeviceUnique(deviceInfo);
	queue = device->getQueue(queueFamily, 0);

	// We will just keep a command buffer for each frame and reset them at the start of the fram
	vk::CommandPoolCreateInfo poolInfo{};
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;
	poolInfo.queueFamilyIndex = queueFamily;
	commandPool = device->createCommandPoolUnique(poolInfo);

	// Initialize per-frame state
	vk::CommandBufferAllocateInfo perFrameCommandBufferInfo{};
	perFrameCommandBufferInfo.commandPool = *commandPool;
	perFrameCommandBufferInfo.commandBufferCount = 1;
	for (PerFrame &pf: perFrame) {
		// Start signalled to indicate the frame is ready to be rendered
		pf.frameFence = device->createFenceUnique({vk::FenceCreateFlagBits::eSignaled});
		pf.acquireImageSemaphore = device->createSemaphoreUnique({});
		pf.submitSemaphore = device->createSemaphoreUnique({});
		pf.commandBuffer = device->allocateCommandBuffers(perFrameCommandBufferInfo).at(0);
	}
}


// Set surface and create swap chain
void VulkanState::setSurface(VkSurfaceKHR surface) {
	assertThat(physicalDevice.getSurfaceSupportKHR(queueFamily, surface), "Surface not supported by selected device\n");
	this->surface = surface;
	createSwapchain();
	createRenderpass();
	setupFramebuffers();
}


// Free resources created from the setSurface call
// Should do this before we free the surface, so can't just rely on the destructor
void VulkanState::unsetSurface() {
	unsetFramebuffers();
	unsetRenderpass();
	unsetSwapchain();
	surface = nullptr;
}


// Make a render pipeline
Pipeline VulkanState::makePipeline(
	std::vector<uint8_t> vertexShaderCode,
	std::vector<uint8_t> fragmentShaderCode,
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo,
	vk::PrimitiveTopology topology,
	size_t pushConstantSize
) {
	assertThat(renderpass, "Surface must be set before making pipeline\n");
	auto vertexModule = makeShaderModule(vertexShaderCode);
	auto fragmentModule = makeShaderModule(fragmentShaderCode);

	vk::PipelineShaderStageCreateInfo vertexStage{};
	vertexStage.stage = vk::ShaderStageFlagBits::eVertex;
	vertexStage.module = *vertexModule;
	vertexStage.pName = "main";
	vk::PipelineShaderStageCreateInfo fragmentStage{};
	fragmentStage.stage = vk::ShaderStageFlagBits::eFragment;
	fragmentStage.module = *fragmentModule;
	fragmentStage.pName = "main";
	std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages { vertexStage, fragmentStage };

	vk::PipelineInputAssemblyStateCreateInfo inputInfo{};
	inputInfo.topology = topology;

	vk::PipelineViewportStateCreateInfo viewportInfo{{}, 1, &viewport, 1, &scissor};

	vk::PipelineRasterizationStateCreateInfo rasterizationInfo{};
	rasterizationInfo.cullMode = vk::CullModeFlagBits::eNone;
	rasterizationInfo.lineWidth = 1.0;

	vk::PipelineMultisampleStateCreateInfo multisampleInfo{};

	vk::PipelineDepthStencilStateCreateInfo depthStencilInfo{};
	depthStencilInfo.depthTestEnable = true;
	depthStencilInfo.depthWriteEnable = true;
	depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;

	vk::PipelineColorBlendAttachmentState blendAttachment{};
	blendAttachment.colorWriteMask = (
		vk::ColorComponentFlagBits::eR |
		vk::ColorComponentFlagBits::eG |
		vk::ColorComponentFlagBits::eB |
		vk::ColorComponentFlagBits::eA
	);

	vk::PipelineColorBlendStateCreateInfo colorBlendInfo{};
	colorBlendInfo.attachmentCount = 1;
	colorBlendInfo.pAttachments = &blendAttachment;

	std::array<vk::DynamicState, 2> dynamicStates {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
	};
	vk::PipelineDynamicStateCreateInfo dynamicStateInfo{};
	dynamicStateInfo.dynamicStateCount = dynamicStates.size();
	dynamicStateInfo.pDynamicStates = dynamicStates.data();

	vk::PushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
	pushConstantRange.offset = 0;
	pushConstantRange.size = pushConstantSize;
	vk::PipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges = &pushConstantRange;

	vk::UniquePipelineLayout pipelineLayout = device->createPipelineLayoutUnique(layoutInfo);

	vk::GraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.stageCount = shaderStages.size();
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputInfo;
	pipelineInfo.pViewportState = &viewportInfo;
	pipelineInfo.pRasterizationState = &rasterizationInfo;
	pipelineInfo.pMultisampleState = &multisampleInfo;
	pipelineInfo.pDepthStencilState = &depthStencilInfo;
	pipelineInfo.pColorBlendState = &colorBlendInfo;
	pipelineInfo.pDynamicState = &dynamicStateInfo;
	pipelineInfo.layout = *pipelineLayout;
	pipelineInfo.renderPass = *renderpass;
	pipelineInfo.basePipelineIndex = -1;

	vk::UniquePipeline pipeline = device->createGraphicsPipelineUnique(nullptr, pipelineInfo);
	return {
		std::move(pipelineLayout),
		std::move(pipeline)
	};
}


// Get next image from the swap chain
// Pretty leaky abstraction, caller must set e.g. set fence
std::optional<std::pair<uint32_t, PerFrame&>> VulkanState::acquireImage() {
	if (shouldRecreateSwapchain) {
		recreateSwapchain();
	}
	PerFrame &frame = perFrame[nextFrame()];
	// Wait if we already have maximum amount of frames in flight
	device->waitForFences(*frame.frameFence, true, UINT64_MAX);
	try {
		uint32_t imageIndex = device->acquireNextImageKHR(*swapchain, UINT64_MAX, *frame.acquireImageSemaphore, nullptr);
		// Could get images out of order, so wait if image is already in use by another frame
		if (swapchainFences[imageIndex]) {
			device->waitForFences(swapchainFences[imageIndex], true, UINT64_MAX);
		}
		swapchainFences[imageIndex] = *frame.frameFence;
		return std::pair<uint32_t, PerFrame&>(imageIndex, frame);
	} catch (vk::OutOfDateKHRError &e) {
		shouldRecreateSwapchain = true;
		return {};
	}
}


// Recreate swap chain before next acquire attempt - call on window resize
void VulkanState::requestRecreateSwapchain() {
	shouldRecreateSwapchain = true;
}


// Create a buffer with inital data
BufferAndMemory VulkanState::createBufferWithData(vk::BufferUsageFlags usage, size_t size, uint8_t *data) {
	BufferAndMemory buffer{};

	vk::BufferCreateInfo bufferInfo{};
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	buffer.buffer = device->createBufferUnique(bufferInfo);

	auto requirements = device->getBufferMemoryRequirements(*buffer.buffer);
	vk::MemoryAllocateInfo allocateInfo{
		requirements.size,
		findMemoryType(
			requirements.memoryTypeBits,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
		)
	};
	buffer.memory = device->allocateMemoryUnique(allocateInfo);

	device->bindBufferMemory(*buffer.buffer, *buffer.memory, 0);

	// Write inital data to buffer
	void *mapped = device->mapMemory(*buffer.memory, 0, size, {});
	memcpy(mapped, data, size);
	device->unmapMemory(*buffer.memory);
	// TODO: should maybe be using a staging buffer to copy to device local memory

	return buffer;
}


void VulkanState::recreateSwapchain() {
	device->waitIdle();

	// Should techhnically recreate render pass and pipelines here too,
	// but is a bit inconvenient with current structure.
	// Should be OK as long as the surface format doesn't change
	unsetFramebuffers();
	unsetSwapchain();
	createSwapchain();
	setupFramebuffers();

	shouldRecreateSwapchain = false;
}


void VulkanState::createSwapchain() {
	auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
	auto formats = physicalDevice.getSurfaceFormatsKHR(surface);
	currentExtent = capabilities.currentExtent;
	if (currentExtent.width == UINT32_MAX) {
		currentExtent.width = std::min(std::max((uint32_t) 800, capabilities.minImageExtent.width), capabilities.maxImageExtent.width);
		currentExtent.height = std::min(std::max((uint32_t) 600, capabilities.minImageExtent.height), capabilities.maxImageExtent.height);
	}

	currentSurfaceFormat = pickFormat(
		formats,
		{vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}
	);

	vk::SwapchainCreateInfoKHR swapchainInfo{};
	swapchainInfo.surface = surface;
	swapchainInfo.minImageCount = capabilities.minImageCount;
	swapchainInfo.imageFormat = currentSurfaceFormat.format;
	swapchainInfo.imageColorSpace = currentSurfaceFormat.colorSpace;
	swapchainInfo.imageExtent = currentExtent;
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	swapchainInfo.presentMode = vk::PresentModeKHR::eFifo;
	swapchainInfo.clipped = true;
	swapchainInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
	swapchainInfo.queueFamilyIndexCount = 0;
	swapchainInfo.pQueueFamilyIndices = nullptr;
	swapchain = device->createSwapchainKHRUnique(swapchainInfo);
	swapchainImages = device->getSwapchainImagesKHR(*swapchain);

	for (auto &image: swapchainImages) {
		swapchainImageViews.emplace_back(
			createImageView(image, currentSurfaceFormat.format, vk::ImageAspectFlagBits::eColor)
		);
		swapchainFences.push_back(nullptr);
	}

	viewport = vk::Viewport(
		0, 0,
		currentExtent.width, currentExtent.height,
		0.0, 1.0
	);
	scissor.offset = vk::Offset2D(0, 0);
	scissor.extent = currentExtent;
}


// Free resources for swap chain
void VulkanState::unsetSwapchain() {
	swapchainFences.clear();
	swapchainImageViews.clear();
	swapchainImages.clear();
	swapchain.reset();
}


void VulkanState::createRenderpass() {
	vk::AttachmentDescription colorAttachment{};
	colorAttachment.format = currentSurfaceFormat.format;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
	vk::AttachmentDescription depthAttachment{};
	// TODO: should detect supported depth format
	depthAttachment.format = vk::Format::eD32Sfloat;
	depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
	depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

	vk::AttachmentReference colorAttachmentRef{0, vk::ImageLayout::eColorAttachmentOptimal};
	vk::AttachmentReference depthAttachmentRef{1, vk::ImageLayout::eDepthStencilAttachmentOptimal};

	vk::SubpassDescription subpass{};
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	vk::SubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.srcAccessMask = {};
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
	vk::RenderPassCreateInfo renderpassInfo{};
	renderpassInfo.attachmentCount = attachments.size();
	renderpassInfo.pAttachments = attachments.data();
	renderpassInfo.subpassCount = 1;
	renderpassInfo.pSubpasses = &subpass;
	renderpass = device->createRenderPassUnique(renderpassInfo);
}


void VulkanState::unsetRenderpass() {
	renderpass.reset();
}


// Set up frame buffers from swap chain - need to do this on init and on resize
void VulkanState::setupFramebuffers() {
	vk::ImageCreateInfo imageInfo{};
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.format = vk::Format::eD32Sfloat;
	imageInfo.extent.width = currentExtent.width;
	imageInfo.extent.height = currentExtent.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment;
	depthImage = device->createImageUnique(imageInfo);

	auto requirements = device->getImageMemoryRequirements(*depthImage);

	vk::MemoryAllocateInfo allocateInfo{};
	allocateInfo.allocationSize = requirements.size,
	allocateInfo.memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

	depthImageMemory = device->allocateMemoryUnique(allocateInfo);

	device->bindImageMemory(*depthImage, *depthImageMemory, 0);

	depthImageView = createImageView(*depthImage, vk::Format::eD32Sfloat, vk::ImageAspectFlagBits::eDepth);

	framebuffers.clear();

	for (auto &image: swapchainImageViews) {
		std::array<vk::ImageView, 2> attachments {
			*image,
			*depthImageView,
		};
		vk::FramebufferCreateInfo framebufferInfo{};
		framebufferInfo.renderPass = *renderpass;
		framebufferInfo.attachmentCount = attachments.size();
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = currentExtent.width;
		framebufferInfo.height = currentExtent.height;
		framebufferInfo.layers = 1;

		framebuffers.emplace_back(device->createFramebufferUnique(framebufferInfo));
	}
}


void VulkanState::unsetFramebuffers() {
	framebuffers.clear();
	depthImageView.reset();
	depthImageMemory.reset();
	depthImage.reset();
}


vk::UniqueImageView VulkanState::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspects) {
	vk::ImageViewCreateInfo imageViewInfo{};
	imageViewInfo.image = image;
	imageViewInfo.format = format;
	imageViewInfo.viewType = vk::ImageViewType::e2D;
	imageViewInfo.subresourceRange.aspectMask = aspects;
	imageViewInfo.subresourceRange.baseMipLevel = 0;
	imageViewInfo.subresourceRange.levelCount = 1;
	imageViewInfo.subresourceRange.baseArrayLayer = 0;
	imageViewInfo.subresourceRange.layerCount = 1;
	return device->createImageViewUnique(imageViewInfo);
}


uint32_t VulkanState::findMemoryType(uint32_t mask, vk::MemoryPropertyFlags requiredProperties) {
	auto memoryProperties = physicalDevice.getMemoryProperties();
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		if ((mask & (1 << i)) &&
		(memoryProperties.memoryTypes[i].propertyFlags & requiredProperties) == requiredProperties) {
			return i;
		}
	}
	assertThat(false, "Could not find usable memory type\n");
}


vk::UniqueShaderModule VulkanState::makeShaderModule(std::vector<uint8_t> &code) {
	vk::ShaderModuleCreateInfo moduleInfo{
		{},
		code.size(),
		reinterpret_cast<const uint32_t*>(code.data())
	};
	return device->createShaderModuleUnique(moduleInfo);
}


size_t VulkanState::nextFrame() {
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	return currentFrame;
}


VulkanState::~VulkanState() {
}