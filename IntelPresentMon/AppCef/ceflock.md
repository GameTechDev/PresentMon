# CEF Lock Guide

`cef-lock.json` records the Chromium Embedded Framework (CEF) distribution used by AppCef. The lock keeps the staged runtime payload reproducible and gives maintainers a clear path for intentional CEF upgrades.

## Legacy Visual Studio Restore

The classic Visual Studio build restores its legacy CEF stage with this
repository PowerShell helper:

```bat
> IntelPresentMon\AppCef\Batch\pull-cef.ps1
```

The script downloads the URI recorded in `IntelPresentMon\AppCef\cef-lock.json`, extracts the archive, rebuilds the CEF C++ wrapper from a clean build directory, stages the AppCef CEF files, and verifies that the staged runtime payload matches the lock file.

The restore never modifies `cef-lock.json`. The current CMake build does not
invoke this helper; its normal Python-backed path is described below.

## Legacy PowerShell Stage Selection

`pull-cef.ps1` and `validate-cef.ps1` accept `-StageKind Legacy|CMake`.
Only these two published destinations exist:

```text
Legacy  IntelPresentMon\AppCef\Cef
CMake   build\ThirdParty\cef
```

`Legacy` is the default, so the command in the previous section and existing
MSBuild callers remain unchanged. For compatibility maintenance, the helper can
also operate on the CMake stage directly:

```bat
> IntelPresentMon\AppCef\Batch\pull-cef.ps1 -StageKind CMake
> IntelPresentMon\AppCef\Batch\validate-cef.ps1 -Mode Stage -StageKind CMake
```

The two-stage selection exists only during migration. When MSBuild is retired,
the `Legacy` stage kind, its callers, and `IntelPresentMon\AppCef\Cef` must be
removed; `build\ThirdParty\cef` then becomes the only published stage.

There is no arbitrary destination parameter. Temporary extraction,
wrapper-build, staging, and retired directories are internal to restore and do
not create another supported published stage. Restore and stage validation are
serialized so validation does not inspect a stage while it is being replaced.

## CMake

The CMake build always uses the shared `build/ThirdParty/cef/<toolset>` stage and never
writes to the Visual Studio stage. Normal configuration validates the stage and
restores it automatically when it is absent or stale:

```bat
> cmake -B build -S . -G "Visual Studio 17 2022"
> cmake --build build --config Debug
```

For Visual Studio 2026 (Experimental), use its distinct binary tree:

```bat
> cmake -B build-vs2026 -S . -G "Visual Studio 18 2026"
> cmake --build build-vs2026 --config Debug
```

`PMON_CEF_SOURCE` optionally supplies the URI recorded in the lock or a local
archive. An alternate URI is rejected. A local archive must match the SHA-256
in `cef-lock.json`; an extracted directory is not accepted for the CMake stage.
The locked URI is used when the variable is empty. For a network-independent
configuration, supply the matching local archive:

```bat
> cmake -B build -S . -G "Visual Studio 17 2022" -DPMON_CEF_SOURCE:FILEPATH=C:\path\to\cef_archive.tar.bz2
> cmake --build build --config Debug
```

`pmon_stage_cef_runtime` incrementally copies the locked runtime files into the
build output. Configure-time validation and `pmon_restore_cef` verify the shared
stage before it is used or published.
`pmon_verify_cef` is the explicit offline CI check for the published CMake stage
and its staged runtime payload.

The explicit restore and verification targets remain available for maintenance
and CI. Runtime staging becomes part of the normal build when a product target
uses CEF. Archives, wrapper-build inputs, and the published CEF SDK are shared
by all CMake build directories; only runnable output files are copied into each
configuration's output directory.

`pull-cef.ps1` also accepts `-Generator`, `-Platform`, and `-Toolset` for the
wrapper build. Their defaults are `Visual Studio 17 2022`, `x64`, and `v143`.
Passing an empty platform or toolset omits the corresponding CMake generator
option.

## Upgrade Locked CEF

Maintainers intentionally updating CEF should use the upgrade path:

```bat
> IntelPresentMon\AppCef\Batch\upgrade-cef.ps1 https://example.com/path/to/cef_archive.tar.bz2
```

The upgrade script downloads the archive, stages the new payload, updates `IntelPresentMon\AppCef\cef-lock.json`, and regenerates the installer CEF WiX fragments.

Review all generated changes with the CEF upgrade.

## Work Directory

CEF download, extraction, and wrapper build work directories are created under a short temporary root by default, normally `C:\pcef` with a fallback to the system temp directory.

To use a different root, set `PRESENTMON_CEF_WORK_ROOT` before running `pull-cef.ps1` or `upgrade-cef.ps1`:

```bat
> set PRESENTMON_CEF_WORK_ROOT=D:\cef-work
> IntelPresentMon\AppCef\Batch\upgrade-cef.ps1 https://example.com/path/to/cef_archive.tar.bz2
```

After a successful pull or upgrade, temporary CEF work directories created by the script are removed by default. Failed runs leave those directories in place for diagnosis.

To keep work directories after a successful run, set `PRESENTMON_CEF_KEEP_WORK=1`:

```bat
> set PRESENTMON_CEF_KEEP_WORK=1
> IntelPresentMon\AppCef\Batch\upgrade-cef.ps1 https://example.com/path/to/cef_archive.tar.bz2
```

## Local Archives

As a fallback, both scripts can accept a local archive path:

```bat
> IntelPresentMon\AppCef\Batch\pull-cef.ps1 path\to\cef_archive.tar.bz2
> IntelPresentMon\AppCef\Batch\pull-cef.ps1 path\to\cef_archive.tar.bz2 -StageKind CMake
> IntelPresentMon\AppCef\Batch\upgrade-cef.ps1 path\to\cef_archive.tar.bz2
```

Only the legacy pull workflow retains its extracted-directory fallback. Lock
upgrades require a URI or archive so every new lock records an archive SHA-256.
The CMake-stage pull accepts only the locked URI or a matching archive.

If CEF was upgraded from a local archive instead of a URI, `pull-cef.ps1` must also be given a matching local archive path.
