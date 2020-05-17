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
	vk::UniqueSwapchainKHR swapchain{};
	std::vector<vk::Image> swapchainImages{};
	std::vector<vk::UniqueImageView> swapchainImageViews{};
	std::vector<vk::Fence> swapchainFences{};
	vk::Viewport viewport{};
	vk::Rect2D scissor{};
	vk::UniquePipelineLayout pipelineLayout{};
	vk::UniqueRenderPass renderpass{};
	vk::UniqueImage depthImage{};
	vk::UniqueDeviceMemory depthImageMemory{};
	vk::UniqueImageView depthImageView{};
	std::vector<vk::UniqueFramebuffer> framebuffers{};
	std::array<PerFrame, MAX_FRAMES_IN_FLIGHT> perFrame{};
	size_t currentFrame{MAX_FRAMES_IN_FLIGHT - 1};

	~VulkanState();

	void init();
	void setSurface(VkSurfaceKHR surface);
	void unsetSurface();
	vk::UniquePipeline makePipeline(std::vector<uint8_t> vertexShaderCode, std::vector<uint8_t> fragmentShaderCode, vk::PrimitiveTopology topology);
	BufferAndMemory createBufferWithData(vk::BufferUsageFlags usage, size_t size, uint8_t *data);
	std::pair<uint32_t, PerFrame&> acquireImage();

private:
	void setupFramebuffers(vk::Extent2D dimensions);
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


struct Model {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};


struct UploadedModel {
	BufferAndMemory vertices;
	BufferAndMemory indices;

	static UploadedModel fromModel(Model &model, VulkanState &vulkan);
};