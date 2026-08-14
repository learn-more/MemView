# Code signing

Release binaries are Authenticode signed with a Certum code signing certificate, through
[learn-more/certum-signing](https://github.com/learn-more/certum-signing). How that works - the cloud HSM, the
SimplySign session, the TOTP login, where to get each value - is documented there, in
[SIGNING.md](https://github.com/learn-more/certum-signing/blob/main/SIGNING.md). This file is only what is
specific to MemView.

**Signing is optional.** Without the settings below a release still builds; it just comes out unsigned and says
so, in an annotation, the job summary, and the release notes. Nothing has to be configured to cut a release.

Only [`release.yml`](workflows/release.yml) signs. [`msbuild.yml`](workflows/msbuild.yml) does not, on purpose:
pull request builds from forks cannot see secrets anyway, and every signature costs a login to a certificate
that is meant for artifacts people actually download.

## What to store

**Settings → Secrets and variables → Actions → Secrets**

| Secret | What it is |
| --- | --- |
| `CERTUM_USERNAME` | The SimplySign account name. |
| `CERTUM_OTP_URI` | The whole `otpauth://totp/...` URI from SimplySign enrolment. |

**Settings → Secrets and variables → Actions → Variables**

| Variable | Required | Default |
| --- | --- | --- |
| `CERTUM_CERTIFICATE_SHA1` | yes | - |
| `CERTUM_TIMESTAMP_SERVER` | no | `http://time.certum.pl` |
| `CERTUM_INTERMEDIATE_URL` | no | `https://repository.certum.pl/ccsca2021.cer` |

All three of the first are required together; missing any one turns signing off for the whole workflow. The
thumbprint is a variable rather than a secret because a certificate thumbprint is public information, and a wrong
one is much easier to spot in a log than a masked `***`.

These are stored per repository. Renewing the certificate changes `CERTUM_CERTIFICATE_SHA1` here and in every
other repository that signs.

## What gets signed, and in what order

Two files: `MemView-x86.exe` and `MemView-x64.exe`. Nothing is embedded in either, so the ordering is the simple
one - build both platforms, then open the SimplySign session, then sign. That keeps the session as short as it
can be: one TOTP code has to cover the signing only, not the builds in front of it.

1. **Build** x86 and x64 into `bin\`.
2. **Sign and report** - `certum-signing@v1`, which connects, signs, and then reads the result back off the
   files. The run fails here if signing was configured and did not take.
3. **Package** one zip per platform, each with its executable and PDB - *after* signing, because signing
   rewrites the executable and what a user runs is whatever came out of the zip. The PDBs carry no signature.

## One login per build

A login spends a one-time code, so there must be exactly one per build. Two inside the same 30-second TOTP step
present the *same* code twice, which Certum may refuse - surfacing as a release that fails to sign for no visible
reason, on some runs and not others.

Hence `release.yml` builds x86 and x64 as two steps in **one job**, rather than a `platform` matrix. `msbuild.yml`
does use a matrix, because nothing there signs.

## What the user sees in the prompt

`signtool /d` sets the program name in the UAC and SmartScreen dialogs. MemView has a version resource, so that
string comes from its own `FileDescription` in [`res/MemView.rc2`](../res/MemView.rc2) - *"Show all mapped memory
in a process"* - and `release.yml` passes no `description:` at all. Changing the wording is a change to the
resource script, in one place. (JobDebug, which has no version resource, has to pass one instead.)

## The toolset

MemView targets Windows XP on both platforms: x86 links with subsystem 5.01, x64 with 5.02. That needs the
`v141_xp` toolset and the 7.1A SDK, which no GitHub-hosted runner image ships by default - both workflows install
them with the `Microsoft.VisualStudio.Component.WinXP` Visual Studio component before building, and pin
`runs-on: windows-2022` because that step uses a hard-coded 2022 install path.

Before the move to GitHub Actions this was `v140_xp` on AppVeyor's "Visual Studio 2015" image. No current runner
image has the VS2015 toolset and windows-2019 is retired, so the project was retargeted to `v141_xp` with
`WindowsTargetPlatformVersion` 7.0. Visual Studio warns that XP targeting is deprecated (MSB8051); when it is
finally removed, that is the point at which MemView either drops XP or stops building on hosted runners.
