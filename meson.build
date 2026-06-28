project('glossy-msp430', 'c', 'cpp',
    default_options: [
        'warning_level=3',
    ],
    license: 'MIT',
    meson_version: '>=1.0.0',
)

target_name = get_option('target')
cmsis_root = get_option('cmsis_root')
if cmsis_root == ''
    cmsis_root = meson.project_source_root() / 'cmsis'
endif

if target_name == 'funclets'
    subdir('Funclets')
elif target_name == 'tools'
    subdir('ChipInfo')
elif target_name == 'all'
    message('Building all ARM firmware targets requires per-target cross files.')
    message('Use individual target: -Dtarget=bluepill / g431kb / stlinv2')
    subdir('Funclets')
    subdir('ChipInfo')
else
    subdir('Firmware.shared')
    if target_name == 'bluepill'
        subdir('target.bluepill')
    elif target_name == 'g431kb'
        subdir('target.bluepill.g431kb')
    elif target_name == 'stlinv2'
        subdir('target.stlinv2')
    else
        error('Unknown target: ' + target_name)
    endif
endif
