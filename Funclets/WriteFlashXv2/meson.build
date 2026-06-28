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
    '-Wl,-Map=' + meson.current_build_dir() / 'WriteFlashXv2.map',
]

writeflashxv2_elf = executable('WriteFlashXv2',
    project_src / 'Funclets/WriteFlashXv2/WriteFlashXv2.cpp',
    cpp_args: msp430_args + ['-Os'],
    link_args: link_args,
    name_suffix: 'elf',
    pie: false,
)

writeflashxv2_bin = custom_target('WriteFlashXv2.bin',
    input: writeflashxv2_elf,
    output: 'WriteFlashXv2.bin',
    command: ['msp430-elf-objcopy', '-O', 'binary', '@INPUT@', '@OUTPUT@'],
    build_by_default: true,
)

writeflashxv2_asm = custom_target('WriteFlashXv2.asm',
    input: writeflashxv2_elf,
    output: 'WriteFlashXv2.ASM',
    command: ['msp430-elf-objdump', '-S', '-d', '@INPUT@'],
    capture: true,
    build_by_default: true,
)
