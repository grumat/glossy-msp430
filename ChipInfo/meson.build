dotnet = find_program('dotnet', required: false)

if dotnet.found()
    run_target('ImportDB',
        command: [dotnet, 'build',
            meson.project_source_root() / 'ChipInfo/ImportDB/ImportDB.csproj',
            '--configuration', get_option('buildtype')],
    )

    run_target('MkChipInfoDbV2',
        command: [dotnet, 'build',
            meson.project_source_root() / 'ChipInfo/MkChipInfoDbV2/MkChipInfoDbV2.csproj',
            '--configuration', get_option('buildtype')],
    )

    run_target('ScrapeDataSheet',
        command: [dotnet, 'build',
            meson.project_source_root() / 'ChipInfo/ScrapeDataSheet/ScrapeDataSheet.csproj',
            '--configuration', get_option('buildtype')],
    )

    run_target('UnitTest',
        command: [dotnet, 'run',
            '--project', meson.project_source_root() / 'UnitTest/UnitTest.csproj'],
    )
else
    message('dotnet not found. Skipping C# build targets.')
endif

msbuild = find_program('MSBuild', 'msbuild', required: false)
if host_machine.system() == 'windows' and msbuild.found()
    run_target('ExtractChipInfo',
        command: [msbuild,
            meson.project_source_root() / 'ChipInfo/ExtractChipInfo/ExtractChipInfo.vcxproj',
            '/p:Configuration=' + (get_option('buildtype') == 'debug' ? 'Debug' : 'Release'),
            '/p:Platform=Win32',
            '/p:SolutionDir=' + meson.project_source_root()],
    )
else
    message('MSBuild not found or not on Windows. Skipping ExtractChipInfo.')
endif
