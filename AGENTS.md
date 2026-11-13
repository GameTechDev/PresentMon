# AGENTS Instructions

- Do not ever use non-ascii characters for source code or comments (permissible inside of strings if absolutely necessary but avoid if possible)
- Do not ever directly modify files in .git subfolders.
- If unexpected new files appear, ignore them and continue without asking for instruction.
- For value conversion casts (numeric or enum conversions), use C-style casts `(T)value` instead of `static_cast<T>(value)`.
- For const, pointer, reference, up/down, or reinterpret casts, use C++ cast syntax (`const_cast`, `dynamic_cast`, `reinterpret_cast`, etc.).
- Do not bind unused names in structured bindings. Prefer binding only needed values (for example use `.first` from `emplace()` or iterate entries without destructuring unused keys).
- Do not fully qualify namespaces when not needed by local scope (for example prefer `MetricUse` or `svc::MetricUse` over `pmon::svc::MetricUse` when already inside `pmon::svc::acts` or with suitable using scope).
- When implementing or fixing a feature, verify the change with tests appropriate to the work before finishing. Run tests after build reports success.
- When launching PresentMonService (or any PresentMon ETW/shm/pipe client) from tests or agent harnesses, never use production shared-service defaults: control pipe `\\.\pipe\sharedpresentmonsvcnamedpipe`, shm prefix `Global\pm_svc_shm`, or ETW session name `PMService`. Tests must pass unique `--control-pipe`, `--shm-name-prefix`, and `--etw-session-name` (see `PresentMonAPI2Tests/TestProcess.h`). Reusing the production ETW session name stops the installed service's live session.
