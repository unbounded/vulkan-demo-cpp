glslc = find_program('glslc')

shaders = [
	'terrain.vert',
	'terrain.frag',
]

shader_targets = []
foreach s : shaders
	shaders += custom_target(
		'shader @0@'.format(s),
		command: [glslc, '@INPUT@', '-o',  '@OUTPUT@'],
		input: s, 
		output: '@PLAINNAME@.spv',
		build_by_default: true,
	)
endforeach