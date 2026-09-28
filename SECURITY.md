# Security Policy

Intel is committed to rapidly addressing security vulnerabilities affecting
our customers and providing clear guidance on the solution, impact, severity,
and mitigation.

## Security model

PresentMon is a local performance measurement and telemetry product. Some
PresentMon components run with elevated privileges so that they can collect
system-wide frame and hardware telemetry and make that telemetry available
through the PresentMon API.

Locally authenticated users are an intended audience for the API and its
exported telemetry. In particular:

- Frame timing, frame rate, target process identifiers and names associated
  with exported performance telemetry, and hardware performance metrics
  exported through the PresentMon API or its read-only shared memory are not
  treated as confidential between local authenticated users.
- A PresentMon client session is not a tenant or confidentiality boundary.
  One local user's ability to obtain telemetry requested or consumed by
  another local user is not, by itself, a security vulnerability.
- PresentMon supports multiple concurrent clients, but it does not treat
  those clients as mutually isolated security tenants or provide SLA-backed
  telemetry availability on shared hosts.
- Shared-memory names, salts, process identifiers, build identifiers, and
  similar values used to locate or describe exported telemetry are not
  secrets or authentication credentials.
- The ability of a local authenticated user to request collection of the
  telemetry exposed by the public API, including telemetry for a process
  owned by another user, is intended behavior.

## Local code execution and telemetry availability

PresentMon relies on the operating system to prevent unauthorized local code
execution. For purposes of PresentMon's threat model, a host on which an
attacker can execute arbitrary code and commands as a locally authenticated
user is already compromised at that user's privilege level.

PresentMon therefore does not treat the availability or correctness of
intentionally exported telemetry as a security boundary on such a host. An
attacker can disrupt the measured workload, clients, IPC, resources, or
telemetry inputs through mechanisms outside PresentMon's control, so
hardening one service or IPC path cannot guarantee end-to-end frame-rate
telemetry availability. Cross-session isolation likewise cannot protect
telemetry that every local authenticated user is independently authorized to
request.

Preserving access to a non-security-critical performance metric under these
conditions would not reduce the attacker's authority, restore a compromised
security boundary, protect sensitive information, or prevent privilege
escalation. It would therefore not meaningfully improve the security posture
of the host.

This does not diminish the remaining security boundaries: privilege
escalation, code execution in a more-privileged process, and disclosure of
information that PresentMon does not intentionally export remain security
vulnerabilities. Consequently, reports whose only demonstrated impact is
disruption, exhaustion, manipulation, or cross-session observation of
intentionally exported PresentMon telemetry are not considered security
vulnerabilities.

This model does not grant clients general access to the privileges of an
elevated PresentMon component. Elevated components must not act as confused
deputies for capabilities outside the intended telemetry interface. For
example, privileged data that PresentMon does not intentionally export,
privileged file or system modifications, and code execution in a more
privileged process remain security boundaries.

## Issues considered security vulnerabilities

Examples of issues that are in scope include:

- Remote code execution, including memory corruption demonstrated to result
  in code execution, in a PresentMon component.
- Escalation from a standard user to an administrator, LocalSystem, or another
  security principal.
- A less-privileged process demonstrably obtaining code execution in a
  more-privileged PresentMon process or client.
- Use of an elevated PresentMon component to perform an operation that is
  outside the intended telemetry interface and that the caller is not
  otherwise authorized to perform. Examples include accessing privileged ETW
  data, writing to protected locations, or changing protected system state.
- Disclosure of credentials, private user content, privileged system data, or
  other information that PresentMon does not intentionally export to local
  authenticated users.
- Unauthorized modification of protected data or security-relevant system
  state.
- Security-impacting access from a remote or unauthenticated context that is
  not part of the documented product design.

## Issues not normally considered security vulnerabilities

The following are normally outside the security scope when performed by a
local authenticated user and when the demonstrated impact is limited to
PresentMon:

- Denial of service against PresentMon, its UI, its API, client admission, a
  telemetry session, or telemetry for a selected process.
- Resource exhaustion, malformed or incomplete IPC, retained object handles,
  stale named objects, or similar behavior that prevents PresentMon from
  creating or servicing later sessions.
- Reading frame rate, frame timing, target process identifiers and names,
  hardware metrics, or other telemetry that PresentMon intentionally exports
  to local authenticated users, including telemetry associated with another
  user's process or client session.
- Opening or enumerating PresentMon's read-only telemetry shared memory.
- Connecting to a local PresentMon IPC endpoint or invoking documented
  telemetry operations, without an additional security-boundary impact.
- Spoofing, redirecting, or disrupting telemetry where the only demonstrated
  consequence is incorrect or unavailable PresentMon output in the attacker's
  local environment.
- Missing per-client isolation, authentication, quotas, timeouts, or rate
  limits where those controls would only protect the availability or
  confidentiality of the non-sensitive telemetry described above.

These behaviors may still be product robustness defects. We may address them
as reliability or defense-in-depth improvements, but they should not be
reported as security vulnerabilities without a separate security-boundary
impact.

## Cases evaluated separately

The underlying mechanism alone does not determine whether a report is in
scope. A report about an otherwise out-of-scope IPC, shared-memory, or denial
of service behavior may be in scope if it demonstrates an additional impact,
such as:

- Code execution across a privilege boundary.
- Disclosure of data that is not part of PresentMon's intended telemetry
  output.
- A privileged operation that is not an intended part of the PresentMon API.
- Persistent modification or exhaustion of protected operating-system
  resources.
- Material impact on operating-system availability or on a security control,
  rather than availability of PresentMon telemetry alone.

Reports should demonstrate the additional impact rather than rely only on the
possibility of further exploitation. Each report is evaluated on the affected
version, deployment configuration, and demonstrated security boundary.

## Reporting a vulnerability

Please report potential security vulnerabilities using the
[Intel security vulnerability reporting process](https://www.intel.com/content/www/us/en/security-center/vulnerability-handling-guidelines.html).

To help us evaluate a report, please include:

- The affected PresentMon version and deployment configuration.
- The attacker's initial privilege level and required preconditions.
- The security boundary crossed and the capability or data gained.
- A minimal reproduction on the latest available version.
- The incremental security impact beyond access to intentionally exported
  telemetry or loss of PresentMon telemetry availability.

Do not include sensitive data from systems you do not own or have permission
to test.

