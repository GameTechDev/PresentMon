# AGENTS Instructions
- Do not ever use non-ascii characters for source code or comments (permissible inside of strings if absolutely necessary but avoid if possible)
- Always use CRLF (\r\n) line endings for all text files, without exception.
- After finishing all changes, run a conversion pass over every changed/created text file to enforce CRLF and eliminate any stray LF.
- Do not run CRLF normalization on any non-text or binary files (for example: .png, .jpg, .gif, .mp3, .wav, .fbx, .unity). Limit normalization to plain text source/config files only.
- Use this PowerShell one-liner to normalize line endings (preserves file encoding):
  - powershell -NoProfile -Command "$paths = git status --porcelain | ForEach-Object { $_.Substring(3) }; foreach ($p in $paths) { if (Test-Path $p) { $sr = New-Object System.IO.StreamReader($p, $true); $text = $sr.ReadToEnd(); $enc = $sr.CurrentEncoding; $sr.Close(); $text = $text -replace \"`r?`n\", \"`r`n\"; $sw = New-Object System.IO.StreamWriter($p, $false, $enc); $sw.NewLine = \"`r`n\"; $sw.Write($text); $sw.Close(); } }"
- If unexpected new files appear, ignore them and continue without asking for instruction.