Param()  # No parameters for now

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# Ensure the "generated" folder exists
if (-not (Test-Path -LiteralPath 'generated')) {
    New-Item -ItemType Directory -Path 'generated' | Out-Null
}

if (Test-Path 'generated\build_id.h') {
    # ------------------------------------------------------------------------------
    # 1) CAPTURE SIGNATURE A (LIST OF UNCOMMITTED CHANGES WITH LINES ADDED/REMOVED)
    # ------------------------------------------------------------------------------
    Write-Host "[build_id_gen] Gathering diff info (Signature A)..."

    # 1A) Normal 'numstat' lines for tracked changes
    $signatureA_current_lines = git diff --numstat

    # 1B) Collect untracked files so we can treat them like "newly added"
    #     (i.e., lines = total lines, removed = 0)
    $untrackedFiles = git ls-files --others --exclude-standard

    foreach ($uf in $untrackedFiles) {
        # Ensure file still exists (and isn't a folder)
        if (Test-Path $uf) {
            # Count lines. If the file is huge or binary, consider try/catch or skip.
            $lineCount = (Get-Content $uf -ErrorAction SilentlyContinue | Measure-Object -Line).Lines

            # Append path to make relative to solution (repo) root
            $uf = 'IntelPresentMon/Versioning/' + $uf

            # Append a "numstat-like" line: "<lines> 0 <filePath>"
            # Use backtick-t (`t) for tabs, to mirror git diff --numstat output.
            $signatureA_current_lines += "$lineCount`t0`t$uf"
        }
    }

    # 1C) Build a single signature string (space-separated or however you like).
    #     If you want each record separated by a newline instead, use "-join [Environment]::NewLine"
    $signatureA_current = $signatureA_current_lines -join ' '

    # Retrieve previous signature A if it exists
    $signatureA_prev = ''
    if (Test-Path 'generated\signature_a_prev.txt') {
        $signatureA_prev = Get-Content 'generated\signature_a_prev.txt' -Raw
    }

    # ------------------------------------------------------------------------------
    # 2) CAPTURE SIGNATURE B (PER-FILE MD5 FOR ALL UNCOMMITTED FILES)
    # ------------------------------------------------------------------------------
    Write-Host "[build_id_gen] Calculating per-file hashes (Signature B)..."
    $signatureB_current = ''

    foreach ($line in $signatureA_current_lines) {
        # Each line is something like: 10  2  src\main.cpp
        # Split on whitespace
        $fields = $line -split '\s+'
        if ($fields.Count -ge 3) {
            $linesAdded   = $fields[0]
            $linesRemoved = $fields[1]
            $changedFile  = '../../' + $fields[2]

            # Edge case: file might be deleted or not present
            if (-not (Test-Path $changedFile)) {
                # Write-Host "[build_id_gen] File '$changedFile' does not exist (possibly deleted). Skipping..."
                continue
            }

            # Get MD5 hash of the file
            # (You can change -Algorithm to e.g. SHA256 if desired)
            $hash = (Get-FileHash $changedFile -Algorithm MD5).Hash
            $signatureB_current += $hash
        }
    }

    # Retrieve previous signature B if it exists s
    $signatureB_prev = ''
    if (Test-Path 'generated\signature_b_prev.txt') {
        $signatureB_prev = Get-Content 'generated\signature_b_prev.txt' -Raw
    }

    # Store the updated Signature B
    $signatureB_current | Set-Content 'generated\signature_b_prev.txt'

    # If Signature A is unchanged, we must check Signature B
    if ($signatureA_current.Trim() -eq $signatureA_prev.Trim()) {
        # If Signature B is unchanged, skip generating build_id.h
        if ($signatureB_current.Trim() -eq $signatureB_prev.Trim()) {
            Write-Host "[build_id_gen] Signature B is unchanged. Skipping Versioning\generated\build_id.h regeneration."
            return
        }    
        Write-Host "[build_id_gen] Signature B mismatch detected."
    } else {
        # Store the updated Signature A
        $signatureA_current | Set-Content 'generated\signature_a_prev.txt'
        Write-Host "[build_id_gen] Signature A mismatch detected."
    }
} else {
    Write-Host "[build_id_gen] Versioning\generated\build_id.h not present; generating..."
}


# ------------------------------------------------------------------------------
# 3) GENERATE/UPDATE BUILD_ID.H
# ------------------------------------------------------------------------------
Write-Host "[build_id_gen] Generating new build_id.h..."

# Get the current git commit hash
$gitHash       = (git rev-parse HEAD)        | Out-String
$gitHashShort  = (git rev-parse --short HEAD)| Out-String
$gitHash       = $gitHash.Trim()
$gitHashShort  = $gitHashShort.Trim()

# Format: yyyy.M.d.HH:mm:ss (e.g., 2025.1.8.12:20:11)
$dateTime = Get-Date -Format 'yyyy.M.d.HH:mm:ss'

# Check if there are uncommitted changes (sets $LASTEXITCODE = 0 if none)
git diff --quiet
if ($LASTEXITCODE -eq 0) {
    $uncommittedChanges = 'false'
} else {
    $uncommittedChanges = 'true'
}

@"
#pragma once
// Auto-generated by build_id_gen.ps1

#define PM_BID_GIT_HASH "$gitHash"
#define PM_BID_GIT_HASH_SHORT "$gitHashShort"
#define PM_BID_TIME "$dateTime"
#define PM_BID_UID "$gitHash-$dateTime"
#define PM_BID_DIRTY $uncommittedChanges
"@ | Set-Content 'generated\build_id.h'