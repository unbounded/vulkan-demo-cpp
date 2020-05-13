
#include <cstdio>
#include <filesystem>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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

	glfwSetKeyCallback(window, glfwKeyCallback);

	while (!glfwWindowShouldClose(window)) {

		glfwPollEvents();
	}

	glfwDestroyWindow(window);

	glfwTerminate();
}
