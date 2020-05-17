
#include <cstdio>
#include <filesystem>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
// not necessary since glm 0.9.6 but include for compatibility
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "terrain.hpp"
#include "vulkan.hpp"
#include "util.h"

namespace fs = std::filesystem;

static void glfwErrorCallback(int error, const char *description) {
	fprintf(stderr, "GLFW error: %s\n", description);
}


static void glfwKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}


static std::vector<uint8_t> readFile(fs::path filename) {
	FILE *file = fopen(filename.c_str(), "rb");
	assertNotNull(file, "could not read shader file\n");
	std::vector<uint8_t> result{};
	int byte;
	while ((byte = getc(file)) != EOF)
		result.push_back(byte);
	fclose(file);
	return result;
};


int main(int argc, char** argv) {
	if (!glfwInit()) {
		fprintf(stderr, "Could not initialize GLFW\n");
		return 1;
	}
	glfwSetErrorCallback(glfwErrorCallback);

	VulkanState vulkan{};
	vulkan.init();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow *window = glfwCreateWindow(600, 600, "Window Title", NULL, NULL);
	assertNotNull(window, "Failed to create window\n");
	glfwSetKeyCallback(window, glfwKeyCallback);

	VkSurfaceKHR surface;
	assertVkSuccess(glfwCreateWindowSurface(*vulkan.instance, window, NULL, &surface));

	vulkan.setSurface(surface);

	fs::path basePath{argv[0]};
	basePath = basePath.parent_path();
	
	auto terrainPipeline = vulkan.makePipeline(
		readFile(basePath / "terrain.vert.spv"),
		readFile(basePath / "terrain.frag.spv"),
		vk::PrimitiveTopology::eTriangleList
	);

	Model terrainModel = makeTerrainModel();
	UploadedModel terrainBuffers = UploadedModel::fromModel(terrainModel, vulkan);

	// TODO: should use a real projection, this is a bit of a hack...
	glm::mat4 projection{};
	// Point y axis up
	projection[1][1] = -1.0;
	// Shorten depth to fit
	projection[2][2] = 0.1;
	// move back a bit
	projection[3][2] = 1.0;

	// TODO: find nicer way to do this
	std::array<vk::ClearValue, 2> clearValues{};
	clearValues[0].color.float32[0] = 1.0;
	clearValues[0].color.float32[1] = 0.0;
	clearValues[0].color.float32[2] = 0.0;
	clearValues[0].color.float32[3] = 0.0;
	clearValues[1].depthStencil.depth = 1.0;
	clearValues[1].depthStencil.stencil = 0;

	double startTime = glfwGetTime();


	while (!glfwWindowShouldClose(window)) {

		auto [framebufferIndex, perFrame] = vulkan.acquireImage();

		double time = glfwGetTime() - startTime;
		glm::mat4 view = glm::lookAt(
			glm::vec3(2 * cos(time), -2.0, 2 * sin(time)),
			glm::vec3(0.0, 0.2, 0.0),
			glm::vec3(0.0, 1.0, 0.0)
		);
		glm::mat4 mvp = projection * view;

		vk::CommandBufferBeginInfo commandBufferInfo{};
		commandBufferInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		perFrame.commandBuffer.begin(commandBufferInfo);

		vk::RenderPassBeginInfo renderPassInfo{};
		renderPassInfo.renderPass = *vulkan.renderpass;
		renderPassInfo.framebuffer = *vulkan.framebuffers.at(framebufferIndex);
		renderPassInfo.renderArea.offset = {{0, 0}};
		renderPassInfo.renderArea.extent = vulkan.currentExtent;
		renderPassInfo.clearValueCount = clearValues.size();
		renderPassInfo.pClearValues = clearValues.data();
		perFrame.commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

		perFrame.commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *terrainPipeline);

		perFrame.commandBuffer.pushConstants(
			*vulkan.pipelineLayout,
			vk::ShaderStageFlagBits::eVertex,
			0,
			sizeof(mvp),
			&mvp
		);
		// TODO: dynamic

		vk::DeviceSize zeroOffset = 0;
		//perFrame.commandBuffer.bindVertexBuffers(0, *(terrainBuffers.vertices.buffer), zeroOffset);
		//perFrame.commandBuffer.bindIndexBuffer(*(terrainBuffers.indices.buffer), zeroOffset, vk::IndexType::eUint32);
		//perFrame.commandBuffer.drawIndexed(terrainModel.indices.size(), 1, 0, 0, 0);

		perFrame.commandBuffer.endRenderPass();
		perFrame.commandBuffer.end();

		vk::SubmitInfo submitInfo{};
		submitInfo.waitSemaphoreCount = 1;
		vk::PipelineStageFlags stageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		submitInfo.pWaitSemaphores = &*perFrame.acquireImageSemaphore;
		submitInfo.pWaitDstStageMask = &stageFlags;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &perFrame.commandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &*perFrame.submitSemaphore;
		vulkan.device->resetFences(*perFrame.frameFence);
		vulkan.queue.submit(submitInfo, *perFrame.frameFence);

		vk::PresentInfoKHR presentInfo{};
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &*perFrame.submitSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &*vulkan.swapchain;
		presentInfo.pImageIndices = &framebufferIndex;
		vulkan.queue.presentKHR(presentInfo);

		glfwPollEvents();
	}

	vulkan.device->waitIdle();
	// TODO: recheck if we need to do this
	//vulkan.instance->destroySurfaceKHR(surface);

	glfwDestroyWindow(window);

	glfwTerminate();
}
