project('neogui', 'c',
        default_options : ['c_std=c11'])

add_global_arguments(language: 'c')

sdl2ttf_lib = meson.get_compiler('c').find_library('SDL2_ttf')
sdl2ttf_dep = declare_dependency(dependencies: [sdl2ttf_lib])
sdl2_dep = dependency('sdl2')
gl_dep = dependency('gl')
harfbuzz_dep = dependency('harfbuzz')

executable('main',
    'native/main.c',
    'native/neogui.c',
    'native/text.c',
    dependencies : [sdl2_dep, sdl2ttf_dep, gl_dep, harfbuzz_dep])
