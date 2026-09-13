# Auxiliary Test Data Guide

Auxiliary test data is stored outside the main repository in the `planetchili/IPMAuxTestData` repository. The main repository pins the expected auxiliary data commit in `Tests\aux-data.lock.json`.

The canonical runtime location is always `Tests\AuxData\Data`. Builds of the
ETL-dependent test projects and their CTest entries validate this data through
the Python auxiliary-data tool. CMake configuration restores the pinned data
automatically when it is absent or stale. No network access occurs when the
local data is already valid.

## Normal CMake Path

The examples use the Windows `py` launcher. On a machine with only
`python.exe`, use `python` wherever `py -3` appears. After installing the Python
requirements, configure and build normally:

```powershell
> py -3 -m pip install -r requirements.txt
> cmake -B build -S . -G "Visual Studio 17 2022"
> cmake --build build --config Debug
```

For Visual Studio 2026 (Experimental), use its distinct binary tree:

```powershell
> cmake -B build-vs2026 -S . -G "Visual Studio 18 2026"
> cmake --build build-vs2026 --config Debug
```

CMake invokes `Scripts\auxdata.py` during configuration and wires the same
verification into the affected build and CTest targets. The tool clones
`IPMAuxTestData` into `Tests\AuxData` if needed, checks out the commit pinned by
`Tests\aux-data.lock.json`, downloads release assets listed by the auxiliary
manifest, extracts them, and verifies their hashes.

To restore or verify only the auxiliary data, run:

```powershell
> py -3 Scripts\auxdata.py restore
> py -3 Scripts\auxdata.py verify
```

Downloaded archives are cached under `build\ThirdParty\downloads\auxdata` and
are shared by all CMake build directories.

## Legacy Manual Pull

To refresh only the auxiliary test data, run:

```powershell
> Tests\pull-aux.ps1
```

By default, the script uses:

- GitHub owner: `planetchili`
- GitHub repository: `IPMAuxTestData`
- local auxiliary repository path: `Tests\AuxData`
- data subdirectory: `Data`
- lock file: `aux-data.lock.json`

The auxiliary repository is intentionally separate from the main repository. Do not commit the downloaded ETL payloads to the main repository.

These PowerShell scripts remain available while the classic DevStudio build is
supported. They are not used by the CMake build graph.

## Updating Auxiliary Data

Maintainers updating auxiliary test data should work in `Tests\AuxData`, then run:

```powershell
> Tests\push-aux.ps1
```

`push-aux.ps1` expects `AUXTESTDATA_GITHUB_TOKEN` to be set. It updates the auxiliary data manifest, commits and pushes changes to the auxiliary data repository, creates release assets for changed ETLs, and writes the new pinned commit to `Tests\aux-data.lock.json` in the main repository.

Review the updated lock file with the auxiliary data change.
