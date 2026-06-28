project_src = meson.project_source_root()

msp430_args = [
    '-mlarge',
    '-msilicon-errata=cpu11,cpu12,cpu13,cpu19',
    '-fPIE',
]

link_args = [
    '-mlarge',
    '-nostartfiles',
    '-fPIE',
    '-Wl,-Map=' + meson.current_build_dir() / 'EraseXv2.map',
]

erasexv2_elf = executable('EraseXv2',
    project_src / 'Funclets/EraseXv2/EraseXv2.cpp',
    cpp_args: msp430_args + ['-Os'],
    link_args: link_args,
    name_suffix: 'elf',
    pie: false,
)

erasexv2_bin = custom_target('EraseXv2.bin',
    input: erasexv2_elf,
    output: 'EraseXv2.bin',
    command: ['msp430-elf-objcopy', '-O', 'binary', '@INPUT@', '@OUTPUT@'],
    build_by_default: true,
)

erasexv2_asm = custom_target('EraseXv2.asm',
    input: erasexv2_elf,
    output: 'EraseXv2.ASM',
    command: ['msp430-elf-objdump', '-S', '-d', '@INPUT@'],
    capture: true,
    build_by_default: true,
)
