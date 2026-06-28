# Security Policy

## Reporting a Vulnerability

The Windows 11 Debloat project takes security seriously. This tool runs with
elevated (`requireAdministrator`) privileges and modifies the Windows registry,
system services, the hosts file, and installed applications — so a
vulnerability in the tool itself can have a real impact.

**Do NOT open a public GitHub issue** to report a security vulnerability.
Public issues can expose exploit details before a fix is available.

Instead, please report vulnerabilities through GitHub's private security
advisory feature:

&rarr; **[https://github.com/RealSyferX/windows-11-debloat/security/advisories/new](https://github.com/RealSyferX/windows-11-debloat/security/advisories/new)**

Please include:

- A description of the vulnerability and its potential impact
- The affected version (see `Debloat.exe --version` or the README)
- Steps to reproduce, including any proof-of-concept
- Your assessment of the attack surface (e.g., backup file tampering,
  PowerShell injection, temp-file TOCTOU)

You can expect an **initial response within 72 hours**. If the report is
accepted, we will work with you on a disclosure timeline and credit you in the
advisory (unless you prefer to remain anonymous).

## Preferred Languages

Please submit reports in **English**.

## Scope

This policy covers vulnerabilities in **the tool itself**, including but not
limited to:

- **Privilege escalation via backup-file tampering** — the tool reads back
  backups (registry values, service start types, performance settings) from
  `%ProgramData%\Debloat\` during Revert operations. A weakness that lets a
  non-admin user modify these files to inject malicious content is in scope.
- **PowerShell injection** — the tool builds PowerShell command strings from
  data-table values. A path that interpolates an unescaped value into a
  single-quoted PS string literal (bypassing `Utils::EscapePsSingleQuote`)
  is in scope.
- **Temp-file TOCTOU / predictable temp paths** — the tool writes PowerShell
  scripts to temp files before execution. A race or predictable-path issue
  that could allow hijacking is in scope.
- **Any other code-execution, privilege-escalation, or data-integrity issue**
  in the binary, its build, or its backup/log handling.

**Out of scope:**

- The Windows features, services, or applications the tool disables or
  removes. We are not responsible for Microsoft's telemetry services,
  bloatware apps, or the design of the Windows registry.
- Issues that require an attacker to already have Administrator rights on the
  machine (the tool itself runs as Administrator by design).
- Behaviour that is already documented in the README as intentional
  (e.g., the three irreversible performance tweaks).

## Backup File Protections

As of **v1.0.0**, all backup and log files written by the tool reside in:

```
%ProgramData%\Debloat\
```

This directory is created with `Utils::CreateSecureDirectory()`, which applies
an ACL granting **Full Control to Administrators and SYSTEM only**. This
prevents non-admin users from tampering with the registry, service, and
performance backups that the elevated tool reads back during Revert. Backup
files are also written atomically (write-to-temp-then-rename via
`Utils::WriteBackupAtomic`) so a crash mid-write cannot corrupt an existing
backup.

If you find a way to bypass these protections, please report it following the
process above.

## Safe Harbor

We support and encourage responsible security research. We will not pursue
legal action against researchers who, in good faith:

- Report a vulnerability through the private advisory process described above,
- Avoid accessing, modifying, or destroying other users' data,
- Do not degrade the availability of the project or its infrastructure,
- Give us reasonable time to remediate before any public disclosure.

This good-faith engagement is greatly appreciated and helps keep users safe.
