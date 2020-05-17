// Helper code and boilerplate for Vulkan setup

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <array>
//#include <vulkan/vulkan.h>

#include "vulkan.hpp"
#include "util.h"


static std::optional<uint32_t> pickQueueFamily(std::vector<vk::QueueFamilyProperties> &queueFamilies) {
	// Pick any queue family that supports graphics
	for (size_t i = 0; i < queueFamilies.size(); i++) {
		if (queueFamilies.at(i).queueFlags & vk::QueueFlagBits::eGraphics)
			return i;
	}
	return {};
}


static vk::SurfaceFormatKHR pickFormat(std::vector<vk::SurfaceFormatKHR> &formats, vk::SurfaceFormatKHR preferredFormat) {
	for (auto &format: formats) {
		if (format == preferredFormat) {
			return format;
		}
	}
	return formats.at(0);
}


void VulkanState::init() {
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
	enabledFeatures.largePoints = true;

	// todo: should verify extension support with physical device
	std::vector<const char*> requiredExtensions {
		"VK_KHR_swapchain",
	};

	vk::DeviceCreateInfo deviceInfo{};
	deviceInfo.pQueueCreateInfos = &queueInfo;
	deviceInfo.pEnabledFeatures = &enabledFeatures;
	deviceInfo.enabledExtensionCount = requiredExtensions.size();
	deviceInfo.ppEnabledExtensionNames = requiredExtensions.data();
	device = physicalDevice.createDeviceUnique(deviceInfo);
	queue = device->getQueue(queueFamily, 0);

	vk::CommandPoolCreateInfo poolInfo{};
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;
	poolInfo.queueFamilyIndex = queueFamily;
	commandPool = device->createCommandPoolUnique(poolInfo);

	vk::CommandBufferAllocateInfo perFrameCommandBufferInfo{};
	perFrameCommandBufferInfo.commandPool = *commandPool;
	perFrameCommandBufferInfo.commandBufferCount = 1;
	for (PerFrame &pf: perFrame) {
		pf.frameFence = device->createFenceUnique({vk::FenceCreateFlagBits::eSignaled});
		pf.acquireImageSemaphore = device->createSemaphoreUnique({});
		pf.submitSemaphore = device->createSemaphoreUnique({});
		pf.commandBuffer = device->allocateCommandBuffers(perFrameCommandBufferInfo).at(0);
	}
}


void VulkanState::setSurface(VkSurfaceKHR surface) {
	this->surface = surface;
	auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(this->surface);
	auto formats = physicalDevice.getSurfaceFormatsKHR(this->surface);
	currentExtent = capabilities.currentExtent;
	if (currentExtent.width == UINT32_MAX) {
		// TODO: should respect min/max extents
		currentExtent = vk::Extent2D{600, 600};
	}

	vk::SurfaceFormatKHR format = pickFormat(
		formats,
		{vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}
	);

	vk::SwapchainCreateInfoKHR swapchainInfo{};
	swapchainInfo.surface = surface;
	swapchainInfo.minImageCount = capabilities.minImageCount;
	swapchainInfo.imageFormat = format.format;
	swapchainInfo.imageColorSpace = format.colorSpace;
	swapchainInfo.imageExtent = currentExtent;
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	swapchainInfo.presentMode = vk::PresentModeKHR::eFifo;
	swapchainInfo.clipped = true;
	// TODO: probably only need color buffer
	swapchainInfo.imageUsage = capabilities.supportedUsageFlags;
	swapchainInfo.queueFamilyIndexCount = 1;
	swapchainInfo.pQueueFamilyIndices = &queueFamily;
	swapchain = device->createSwapchainKHRUnique(swapchainInfo);
	swapchainImages = device->getSwapchainImagesKHR(*swapchain);

	for (auto &image: swapchainImages) {
		swapchainImageViews.emplace_back(
			createImageView(image, format.format, vk::ImageAspectFlagBits::eDepth)
		);
		swapchainFences.push_back(nullptr);
	}

	vk::AttachmentDescription colorAttachment{};
	colorAttachment.format = format.format;
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

	setupFramebuffers(currentExtent);
}

// Set up frame buffers from swap chain - need to do this on init and on resize
void VulkanState::setupFramebuffers(vk::Extent2D dimensions) {
	// TODO: set dynamic state

	vk::ImageCreateInfo imageInfo{};
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.format = vk::Format::eD32Sfloat;
	imageInfo.extent.width = dimensions.width;
	imageInfo.extent.height = dimensions.height;
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
		framebufferInfo.width = dimensions.width;
		framebufferInfo.height = dimensions.height;
		framebufferInfo.layers = 1;

		framebuffers.emplace_back(device->createFramebufferUnique(framebufferInfo));
	}
}


uint32_t VulkanState::findMemoryType(uint32_t mask, vk::MemoryPropertyFlags requiredProperties) {
	auto memoryProperties = physicalDevice.getMemoryProperties();
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		if ((mask & (1 << i)) &&
		(memoryProperties.memoryTypes[i].propertyFlags & requiredProperties) == requiredProperties) {
			return i;
		}
	}
	assertThat(false, "Could not find usable memory type");
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

vk::UniquePipeline VulkanState::makePipeline(std::vector<uint8_t> vertexShaderCode, std::vector<uint8_t> fragmentShaderCode, vk::PrimitiveTopology topology) {
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

	vk::VertexInputBindingDescription vertexBinding{};
	vertexBinding.binding = 0;
	vertexBinding.stride = sizeof(Vertex);
	vertexBinding.inputRate = vk::VertexInputRate::eVertex;
	std::vector<vk::VertexInputAttributeDescription> vertexAttributes{
		{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)},
		{0, 1, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)},
		{0, 2, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)},
	};

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &vertexBinding;
	vertexInputInfo.vertexAttributeDescriptionCount = vertexAttributes.size();
	vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes.data();

	vk::PipelineInputAssemblyStateCreateInfo inputInfo{};
	inputInfo.topology = topology;

	// FIXME use dynamic
	vk::Viewport viewport{
		0, 0,
		600, 600,
		0.0, 1.0
	};
	vk::Rect2D scissor{{0, 0}, {600, 600}};
	vk::PipelineViewportStateCreateInfo viewportInfo{{}, 1, &viewport, 1, &scissor};

	vk::PipelineRasterizationStateCreateInfo rasterizationInfo{};
	// TODO: backface culling
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

	std::array<vk::DynamicState, 1> dynamicStates {
		vk::DynamicState::eViewport,
	};
	vk::PipelineDynamicStateCreateInfo dynamicStateInfo{};
	dynamicStateInfo.dynamicStateCount = dynamicStates.size();
	dynamicStateInfo.pDynamicStates = dynamicStates.data();

	vk::PushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(glm::mat4);
	vk::PipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges = &pushConstantRange;

	pipelineLayout = device->createPipelineLayoutUnique(layoutInfo);

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

	return device->createGraphicsPipelineUnique(nullptr, pipelineInfo);
}

vk::UniqueShaderModule VulkanState::makeShaderModule(std::vector<uint8_t> &code) {
	vk::ShaderModuleCreateInfo moduleInfo{
		{},
		code.size(),
		reinterpret_cast<const uint32_t*>(code.data())
	};
	return device->createShaderModuleUnique(moduleInfo);
}

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


std::pair<uint32_t, PerFrame&> VulkanState::acquireImage() {
	PerFrame &frame = perFrame[nextFrame()];
	// Wait if we already have maximum amount of frames in flight
	device->waitForFences(*frame.frameFence, true, UINT64_MAX);
	uint32_t imageIndex = device->acquireNextImageKHR(*swapchain, UINT64_MAX, *frame.acquireImageSemaphore, nullptr);
	// Could get images out of order, so wait if image is already in use by another frame
	if (swapchainFences[imageIndex]) {
		device->waitForFences(swapchainFences[imageIndex], true, UINT64_MAX);
	}
	swapchainFences[imageIndex] = *frame.frameFence;
	return {imageIndex, frame};
}


size_t VulkanState::nextFrame() {
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	return currentFrame;
}


VulkanState::~VulkanState() {
	if (surface) {
		vkDestroySurfaceKHR(*instance, surface, nullptr);
	}
}


UploadedModel UploadedModel::fromModel(Model &model, VulkanState &vulkan) {
	auto vertices = vulkan.createBufferWithData(
		vk::BufferUsageFlagBits::eVertexBuffer,
		model.vertices.size() * sizeof(model.vertices[0]),
		reinterpret_cast<uint8_t*>(model.vertices.data())
	);
	auto indices = vulkan.createBufferWithData(
		vk::BufferUsageFlagBits::eIndexBuffer,
		model.indices.size() * sizeof(model.indices[0]),
		reinterpret_cast<uint8_t*>(model.indices.data())
	);
	return {
		std::move(vertices),
		std::move(indices)
	};
}