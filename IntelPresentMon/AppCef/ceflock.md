# CEF Lock Guide

`cef-lock.json` records the Chromium Embedded Framework (CEF) distribution used by AppCef. The lock keeps the staged runtime payload reproducible and gives maintainers a clear path for intentional CEF upgrades.

## Restore Locked CEF

From the repository root, run:

```bat
> IntelPresentMon\AppCef\Batch\pull-cef.ps1
```

The script downloads the URI recorded in `IntelPresentMon\AppCef\cef-lock.json`, extracts the archive, rebuilds the CEF C++ wrapper from a clean build directory, stages the AppCef CEF files, and verifies that the staged runtime payload matches the lock file.

The normal restore path never modifies `cef-lock.json`.

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
> IntelPresentMon\AppCef\Batch\upgrade-cef.ps1 path\to\cef_archive.tar.bz2
```

If CEF was upgraded from a local archive instead of a URI, `pull-cef.ps1` must also be given a matching local archive path.
