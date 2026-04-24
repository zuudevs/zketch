# Security Policy

## Supported Versions

Only the latest release of zketch receives security fixes.

| Version | Supported |
|---|---|
| 0.1.x (current) | Yes |

## Reporting a Vulnerability

If you discover a security vulnerability in zketch, please do not open a public GitHub issue. Instead, report it privately so it can be addressed before public disclosure.

**How to report:**

1. Send an email to the maintainers describing the vulnerability. Include:
   - A clear description of the issue
   - Steps to reproduce or a minimal proof-of-concept
   - The potential impact (e.g., memory corruption, privilege escalation, denial of service)
   - The affected version(s)

2. You will receive an acknowledgment within 72 hours.

3. The maintainers will investigate and aim to release a fix within 14 days for critical issues, or 30 days for lower-severity issues.

4. Once a fix is released, the vulnerability will be publicly disclosed with credit to the reporter (unless you prefer to remain anonymous).

## Scope

zketch is a Windows-only GUI framework. Security concerns most relevant to this project include:

- Memory safety issues (buffer overflows, use-after-free, double-free)
- Unsafe handling of untrusted widget config format input via `WidgetParser`
- Incorrect RAII cleanup leading to handle leaks or dangling pointers
- Thread safety violations in the main-thread queue or event dispatcher

## Out of Scope

- Vulnerabilities in third-party dependencies (Catch2, Windows SDK, Direct2D)
- Issues that require physical access to the machine
- Social engineering attacks

## Disclosure Policy

zketch follows a coordinated disclosure model. We ask that you give us a reasonable amount of time to fix the issue before making it public.
