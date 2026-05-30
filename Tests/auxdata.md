# Auxiliary Test Data Guide

Auxiliary test data is stored outside the main repository in the `planetchili/IPMAuxTestData` repository. The main repository pins the expected auxiliary data commit in `Tests\aux-data.lock.json`.

## Bootstrap Path

From the repository root, run:

```powershell
> .\bootstrap.ps1
```

The bootstrap script runs `Tests\pull-aux.ps1`, which clones `IPMAuxTestData` into `Tests\AuxData` if needed, checks out the commit pinned by `Tests\aux-data.lock.json`, downloads ETL release assets listed by the auxiliary manifest, extracts them, and verifies their hashes.

## Manual Pull

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

## Updating Auxiliary Data

Maintainers updating auxiliary test data should work in `Tests\AuxData`, then run:

```powershell
> Tests\push-aux.ps1
```

`push-aux.ps1` expects `AUXTESTDATA_GITHUB_TOKEN` to be set. It updates the auxiliary data manifest, commits and pushes changes to the auxiliary data repository, creates release assets for changed ETLs, and writes the new pinned commit to `Tests\aux-data.lock.json` in the main repository.

Review the updated lock file with the auxiliary data change.
