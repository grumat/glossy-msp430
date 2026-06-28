project_src = meson.project_source_root()

firmware_defines = [
    '-DSTM32F103xB',
    '-DBLUEPILL_V2',
]

buildtype = get_option('buildtype')
if buildtype == 'debug' or buildtype == 'debugoptimized'
    firmware_defines += '-DDEBUG=1'
else
    firmware_defines += '-DNDEBUG=1'
    firmware_defines += '-DRELEASE=1'
endif

firmware_inc = include_directories(
    '.',
    '../Firmware.shared',
    '../bmt/include',
    '../cmsis',
)

arm_args = ['-mcpu=cortex-m3', '-mthumb']
firmware_c_args = arm_args + firmware_defines
firmware_cpp_args = arm_args + firmware_defines + ['-fno-exceptions', '-fno-rtti']

link_args = [
    '-mcpu=cortex-m3', '-mthumb',
    '-Wl,-T' + project_src / 'target.stlinv2/STM32F103CB_flash.lds',
    '--specs=nano.specs',
    '--specs=nosys.specs',
    '-Wl,--no-warn-rwx-segments',
    '-Wl,-Map=' + meson.current_build_dir() / 'target.stlinv2.map',
]

startup = '../cmsis/startup/startup_stm32f103xb.c'
sources = firmware_sources + [startup]

python = find_program('python3', 'python')
objcopy_script = project_src / 'tools/objcopy_binary.py'

erase_o = custom_target('erase.bin.o',
    input: project_src / 'Firmware.shared/res/EraseXv2.bin',
    output: '____Firmware_shared_res_EraseXv2.bin.o',
    command: [
        python, objcopy_script,
        '@INPUT@', '@OUTPUT@',
        '___Firmware_shared_res_EraseXv2.bin',
    ],
    build_by_default: true,
)

write_o = custom_target('writeflash.bin.o',
    input: project_src / 'Firmware.shared/res/WriteFlashXv2.bin',
    output: '____Firmware_shared_res_WriteFlashXv2.bin.o',
    command: [
        python, objcopy_script,
        '@INPUT@', '@OUTPUT@',
        '___Firmware_shared_res_WriteFlashXv2.bin',
    ],
    build_by_default: true,
)

target_elf = executable('target.stlinv2',
    sources, erase_o, write_o,
    c_args: firmware_c_args,
    cpp_args: firmware_cpp_args,
    include_directories: firmware_inc,
    link_args: link_args,
    name_suffix: 'elf',
    pie: false,
    override_options: ['c_std=gnu11', 'cpp_std=gnu++20'],
)

target_bin = custom_target('target.stlinv2.bin',
    input: target_elf,
    output: 'target.stlinv2.bin',
    command: ['arm-none-eabi-objcopy', '-O', 'binary', '@INPUT@', '@OUTPUT@'],
    build_by_default: true,
)

target_hex = custom_target('target.stlinv2.hex',
    input: target_elf,
    output: 'target.stlinv2.hex',
    command: ['arm-none-eabi-objcopy', '-O', 'ihex', '@INPUT@', '@OUTPUT@'],
    build_by_default: true,
)

target_lst = custom_target('target.stlinv2.lst',
    input: target_elf,
    output: 'target.stlinv2.lst',
    command: ['arm-none-eabi-objdump', '-S', '-d', '@INPUT@'],
    capture: true,
    build_by_default: true,
)
