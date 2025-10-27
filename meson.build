project('xg2system', 'c', 'cpp')

core_srcs = [
  'core/log.c',
  'core/log.h',
  'core/mem.c',
  'core/mem.h',
  'core/module_api.h',
  'core/fs.c',
  'core/fs.h',
]

engine_srcs = [
  'engine/module.c',
  'engine/module.h',
  'engine/resource.c',
  'engine/resource.h',
  'engine/runtime.c',
  'engine/runtime.h',
  'engine/transform.c',
  'engine/transform.h',
  'engine/debug.c',
  'engine/debug.h',
]

graphics_srcs = [
  'graphics/camera.c',
  'graphics/camera.h',
  'graphics/image.c',
  'graphics/image.h',
  'graphics/model.c',
  'graphics/model.h',
  'graphics/renderer.c',
  'graphics/renderer.h',
  'graphics/resource.c',
  'graphics/resource.h',
  'graphics/shader_program.c',
  'graphics/shader_program.h',
  'graphics/window.c',
  'graphics/window.h',
]

all_srcs = core_srcs + engine_srcs + graphics_srcs

conf_data = configuration_data()
conf_data.set('version_major', '0')
conf_data.set('version_minor', '0')
conf_data.set('version_revision', '0')
configure_file(input: 'core/config.h.in', output: 'config.h', configuration: conf_data)

engine_incl = include_directories('.')

glib = dependency('glib-2.0', required: true)
flecs = dependency('flecs', required: true)
bgfx = dependency('bgfx', required: true)
cglm = dependency('cglm', required: true)
glfw = dependency('glfw3', required: true)
assimp = dependency('assimp', required: true)


cpp = meson.get_compiler('cpp')

engine_deps = [glib, flecs, bgfx, cglm, cpp.find_library('bx'), cpp.find_library('bimg'), glfw, assimp]

shaderc = find_program('shaderc', required: true)

glsl_gen_args = ['-p', '150']
gen_vs = generator(shaderc,
                output: '@BASENAME@.vs.bin',
                arguments: ['-f', '@INPUT@', '-o', '@OUTPUT@', '--type', 'vertex', glsl_gen_args])
gen_fs = generator(shaderc,
                output: '@BASENAME@.fs.bin',
                arguments: ['-f', '@INPUT@', '-o', '@OUTPUT@', '--type', 'fragment', glsl_gen_args])

library = shared_library('engine', all_srcs, include_directories: engine_incl, dependencies: engine_deps)

engine_dep = declare_dependency(include_directories: engine_incl, dependencies: engine_deps, link_with: library)

executable('engine', 'engine/main.c', dependencies: engine_dep)

gametest_srcs = [
  'gametest/gametest.c'
]

gen_shaders = []
gen_shaders += gen_vs.process(['shaders/example.vs', 'shaders/example_instanced.vs'])
gen_shaders += gen_fs.process(['shaders/example.fs'])

gametest = shared_module('game', gametest_srcs, gen_shaders, dependencies: engine_dep)
