#pragma once
// Helper code and boilerplate for Vulkan setup

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

static const size_t MAX_FRAMES_IN_FLIGHT = 2;

struct BufferAndMemory {
	vk::UniqueBuffer buffer;
	vk::UniqueDeviceMemory memory;
};


struct PerFrame {
	vk::UniqueFence frameFence;
	vk::UniqueSemaphore acquireImageSemaphore;
	vk::UniqueSemaphore submitSemaphore;
	vk::CommandBuffer commandBuffer;
};


struct Pipeline {
	vk::UniquePipelineLayout layout;
	vk::UniquePipeline pipeline;
};


class VulkanState
{
public:
	vk::UniqueInstance instance{};
	vk::PhysicalDevice physicalDevice{};
	uint32_t queueFamily{};
	vk::UniqueDevice device{};
	vk::Queue queue{};
	vk::UniqueCommandPool commandPool{};
	vk::SurfaceKHR surface{};
	vk::Extent2D currentExtent{};
	vk::SurfaceFormatKHR currentSurfaceFormat;
	vk::UniqueSwapchainKHR swapchain{};
	std::vector<vk::Image> swapchainImages{};
	std::vector<vk::UniqueImageView> swapchainImageViews{};
	std::vector<vk::Fence> swapchainFences{};
	vk::Viewport viewport{};
	vk::Rect2D scissor{};
	vk::UniqueRenderPass renderpass{};
	vk::UniqueImage depthImage{};
	vk::UniqueDeviceMemory depthImageMemory{};
	vk::UniqueImageView depthImageView{};
	std::vector<vk::UniqueFramebuffer> framebuffers{};
	std::array<PerFrame, MAX_FRAMES_IN_FLIGHT> perFrame{};
	size_t currentFrame{MAX_FRAMES_IN_FLIGHT - 1};
	bool shouldRecreateSwapchain = false;

	~VulkanState();

	void init();
	void setSurface(VkSurfaceKHR surface);
	void unsetSurface();
	Pipeline makePipeline(
		std::vector<uint8_t> vertexShaderCode,
		std::vector<uint8_t> fragmentShaderCode,
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo,
		vk::PrimitiveTopology topology,
		size_t pushConstantSize
	);
	BufferAndMemory createBufferWithData(vk::BufferUsageFlags usage, size_t size, uint8_t *data);
	std::optional<std::pair<uint32_t, PerFrame&>> acquireImage();
	void requestRecreateSwapchain();

private:
	void recreateSwapchain();
	void createSwapchain();
	void unsetSwapchain();
	void createRenderpass();
	void unsetRenderpass();
	void setupFramebuffers();
	void unsetFramebuffers();
	vk::UniqueImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspects);
	uint32_t findMemoryType(uint32_t mask, vk::MemoryPropertyFlags requiredProperties);
	vk::UniqueShaderModule makeShaderModule(std::vector<uint8_t> &code);
	size_t nextFrame();
};


struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;
};


struct Particle {
	glm::vec3 pos0;
	glm::vec3 v0;
};


struct Model {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};


struct UploadedModel {
	BufferAndMemory vertices;
	BufferAndMemory indices;

	static UploadedModel fromModel(Model &model, VulkanState &vulkan);
};