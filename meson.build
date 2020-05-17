project('vulkan-demo', 'cpp', default_options : ['cpp_std=c++17', 'layout=flat'])

subdir('src/shaders')

glfw = dependency('glfw3')
vulkan = dependency('vulkan')
glm = dependency('glm')

sources = [
	'src/main.cpp',
	'src/model.cpp',
	'src/particles.cpp',
	'src/terrain.cpp',
	'src/vulkan.cpp',
]

executable('vulkan-demo', sources, dependencies: [glfw, vulkan, glm])
