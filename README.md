# Vulkan demo

Small Vulkan test project using the C++ bindings.
Loosely based on https://vulkan-tutorial.com/.

![screenshot](/screenshot.png)

## Requirements

### Tools

* [meson](https://mesonbuild.com/)
* [glslc](https://github.com/google/shaderc)

### Libraries

* [glfw](https://www.glfw.org/)
* [glm](https://glm.g-truc.net/)
* [Vulkan](https://vulkan.lunarg.com/sdk/home)

## Building

```sh
meson build
ninja -C build
```

## Running

```sh
build/meson-out/vulkan-demo
```


## Debugging

Misc useful debugging commands, require the respective tools to be installed.

```sh
# Check for memory errors with Valgrind
valgrind build/meson-out/vulkan-demo

# Enable Vulkan validation layer
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation build/meson-out/vulkan-demo

# Dump Vulkan API calls
VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_api_dump build/meson-out/vulkan-demo
```
