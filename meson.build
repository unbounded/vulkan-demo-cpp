project('vulkan-demo', 'cpp', default_options : ['cpp_std=c++17'])

subdir('src/shaders')

glfw = dependency('glfw3')
vulkan = dependency('vulkan')
glm = dependency('glm')

sources = [
	'src/main.cpp',
	'src/vulkan.cpp',
	'src/terrain.cpp',
	'src/particles.cpp',
]

executable('vulkan-demo', sources, dependencies: [glfw, vulkan, glm])