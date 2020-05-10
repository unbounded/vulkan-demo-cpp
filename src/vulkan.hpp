#pragma once
// Helper code and boilerplate for Vulkan setup

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;
};

class VulkanState
{
public:
	vk::UniqueInstance instance;
	vk::PhysicalDevice physicalDevice;
	uint32_t queueFamily;
	vk::UniqueDevice device;
	vk::Queue queue;
	vk::SurfaceKHR surface;
	vk::UniqueSwapchainKHR swapchain;
	std::vector<vk::Image> swapchainImages;
	std::vector<vk::UniqueImageView> swapchainImageViews;
	vk::UniquePipelineLayout pipelineLayout;
	vk::UniqueRenderPass renderpass;
	vk::UniqueImage depthImage;
	vk::UniqueDeviceMemory depthImageMemory;
	vk::UniqueImageView depthImageView;
	std::vector<vk::UniqueFramebuffer> framebuffers;

	VulkanState() :
		instance(nullptr),
		physicalDevice(),
		queueFamily(),
		device(),
		surface(),
		swapchainImages(),
		swapchainImageViews(),
		pipelineLayout(),
		renderpass(),
		depthImage(),
		depthImageMemory(),
		depthImageView(),
		framebuffers()
		{};
	~VulkanState();

	void init();
	void setSurface(VkSurfaceKHR surface);
	vk::UniquePipeline makePipeline(std::vector<uint8_t> vertexShaderCode, std::vector<uint8_t> fragmentShaderCode, vk::PrimitiveTopology topology);

private:
	void setupFramebuffers(vk::Extent2D dimensions);
	vk::UniqueImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspects);
	uint32_t findMemoryType(uint32_t mask, vk::MemoryPropertyFlags requiredProperties);
	vk::UniqueShaderModule makeShaderModule(std::vector<uint8_t> &code);
};
