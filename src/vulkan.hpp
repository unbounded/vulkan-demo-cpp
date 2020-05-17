#pragma once

// Helper code and boilerplate for Vulkan setup

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>


static const size_t MAX_FRAMES_IN_FLIGHT = 2;


// Buffer and its backing memory
struct BufferAndMemory {
	vk::UniqueBuffer buffer;
	vk::UniqueDeviceMemory memory;
};


// Resources we need one of per in-flight frame
struct PerFrame {
	// Fence that is signalled when the previous frame using this frame structure is finished
	vk::UniqueFence frameFence;
	// Semaphore for acquiring the image from the swap chain
	vk::UniqueSemaphore acquireImageSemaphore;
	// Semaphore for rendering the submitted commands before presenting the result
	vk::UniqueSemaphore submitSemaphore;
	// Command buffer that will be recorded and submitted for each frame
	vk::CommandBuffer commandBuffer;
};


struct Pipeline {
	vk::UniquePipelineLayout layout;
	vk::UniquePipeline pipeline;

	// Free held resources
	void reset() {
		layout.reset();
		pipeline.reset();
	}
};


// Holds generic Vulkan state and set up code
// More a grab bag than a watertight abstraction so most of the stuff is public,
// could be cleaned up a bit.
class VulkanState
{
public:
	// Generic global instances
	vk::UniqueInstance instance{};
	vk::PhysicalDevice physicalDevice{};
	uint32_t queueFamily{};
	vk::UniqueDevice device{};
	vk::Queue queue{};
	vk::UniqueCommandPool commandPool{};

	// Swap chain state
	vk::SurfaceKHR surface{};
	vk::Extent2D currentExtent{};
	vk::SurfaceFormatKHR currentSurfaceFormat;
	vk::UniqueSwapchainKHR swapchain{};
	std::vector<vk::Image> swapchainImages{};
	std::vector<vk::UniqueImageView> swapchainImageViews{};
	std::vector<vk::Fence> swapchainFences{};
	bool shouldRecreateSwapchain = false;

	// Dynamic pipeline state
	vk::Viewport viewport{};
	vk::Rect2D scissor{};

	vk::UniqueRenderPass renderpass{};

	// Depth and frame buffers
	vk::UniqueImage depthImage{};
	vk::UniqueDeviceMemory depthImageMemory{};
	vk::UniqueImageView depthImageView{};
	std::vector<vk::UniqueFramebuffer> framebuffers{};

	// Frame state
	std::array<PerFrame, MAX_FRAMES_IN_FLIGHT> perFrame{};
	size_t currentFrame{MAX_FRAMES_IN_FLIGHT - 1};

	~VulkanState();

	// Initial setup, create device
	void init();

	// Set surface and create swap chain
	void setSurface(VkSurfaceKHR surface);

	// Tear down structures that depend on surface, so application can safely destroy it
	void unsetSurface();

	// Make a render pipeline
	Pipeline makePipeline(
		std::vector<uint8_t> vertexShaderCode,
		std::vector<uint8_t> fragmentShaderCode,
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo,
		vk::PrimitiveTopology topology,
		size_t pushConstantSize
	);

	// Get next image from the swap chain, and frame specific structures
	std::optional<std::pair<uint32_t, PerFrame&>> acquireImage();

	// Recreate swap chain before next acquire attempt - call on window resize
	void requestRecreateSwapchain();

	// Create a buffer with inital data
	BufferAndMemory createBufferWithData(vk::BufferUsageFlags usage, size_t size, uint8_t *data);

private:
	void recreateSwapchain();
	void createSwapchain();
	void unsetSwapchain();
	void createRenderpass();
	void unsetRenderpass();
	void setupFramebuffers();
	void unsetFramebuffers();

	// Create image view for swap chain or depth image
	vk::UniqueImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspects);

	// Find a memory type that satisfies the given properties
	uint32_t findMemoryType(uint32_t mask, vk::MemoryPropertyFlags requiredProperties);

	// Make shader module from binary data
	vk::UniqueShaderModule makeShaderModule(std::vector<uint8_t> &code);

	// Increase current frame and return frame index
	size_t nextFrame();
};