# UCR Internal (executor track) — skeleton

Long-war project: a real in-process executor. This commit is **Phase 1** (framework).
It compiles into an injector + DLL that load, validate, log, and refuse to run on
unsupported clients. It does **not** bypass Hyperion — see below.

## Honest status

| Piece | State |
|---|---|
| Injector CLI (PID find, version check, provider dispatch) | ✅ built |
| Injection / Hyperion bypass provider | ❌ **missing — this is THE wall** |
| DLL core (log, version gate, DataModel resolve, smoke test) | ✅ built |
| In-process SDK (direct instance readers, pattern scan) | ✅ skeleton |
| Demo feature (file-driven WalkSpeed) | ✅ built |
| ScriptContext / Luau state / bytecode pipeline | ❌ Phase 3+ (needs a foothold first) |
| Identity / capabilities / hooks | ❌ Phase 4+ |

## Why the bypass is missing (read this)

Hyperion (Byfron) guards the process with handle stripping, thread/integrity checks,
and a hypervisor. Every working bypass is one of:

- a **private kernel driver** (custom or abused-vulnerable, HVCI/DSE/cat-and-mouse), or
- a **private Hyperion exploit** (worth a fortune, burned the day it's published).

Publishing a working bypass = patched within days. So this repo ships a clean
**provider interface** (`Injector/Src/Bypass/IBypass.h`) and a `NullProvider` that
fails with a clear message. You bring a private primitive; everything else plugs in.
Anyone promising a public copy-paste Hyperion bypass is lying or dated.

## Architecture

```
Injector.exe  --(your provider)-->  RobloxPlayerBeta.exe
      |                                    |
      +-- finds PID, prints client         +-- UCRCore.dll worker thread:
          version, dispatches                   version gate -> resolve DataModel
          to IInjectionProvider                 (proven statics, direct reads)
                                                -> smoke-test services
                                                -> tick internal features
                                                -> log to %TEMP%\UCRInternal.log
```

Key design decisions:

- **External recon, internal action.** The DLL reads `FakeDataModel` at the same
  proven RVA as the external (`0x8d22868` on `version-e7d81637`), but with direct
  dereferences instead of RPM. Same maintenance burden, zero new RE.
- **Fail-safe version gate.** Supported versions are an explicit allowlist in
  `Core/Src/Version.h`. Anything else → log + refuse. Stale statics corrupt memory.
- **SEH-guarded reads.** In-process has no RPM safety net; `Mem::Read/Write` use
  `__try/__except` and return defaults instead of crashing.
- **No UX in the game (yet).** Skeleton logs to a file. Overlay/ImGui-in-DXGI and
  an execution API come after injection works.

## Bypass provider contract

Implement `IInjectionProvider` (`Name`, `IsAvailable`, `Inject(pid, dllPath)`),
register it in `GetProviders()`. Requirements:

1. Must survive Hyperion (that's the whole job — handle/thread/integrity).
2. Must leave the image executable and invoke `DllMain(DLL_PROCESS_ATTACH)`.
3. Must report failures with actionable strings (the CLI prints them).

Ideas that historically worked *as techniques* (all need private engineering now):
driver-backed manual map, thread hijack + shellcode loader, exploiting update races.
Test everything on **alt accounts** — Hyperion issues bans, not kicks, and HWID bans
are real.

## Roadmap

- **Phase 1 (this commit):** skeleton + interface + version-gated core. ✅
- **Phase 2:** private injection provider. Whoever owns the primitive owns this phase.
  Deliverable: `UCRInternal.log` shows resolved DataModel/Players/LocalPlayer.
- **Phase 3:** in-process SDK expansion — port Name sweep to direct reads (same
  oracles as the external), matrix/primitive helpers, service cache.
- **Phase 4:** ScriptContext discovery + Luau state acquisition (needs binary RE per
  version: patterns + offsets, validated at runtime like everything else).
- **Phase 5:** bytecode pipeline — Luau compile → client opcode map → deserializer;
  per-version opcode map is the maintenance treadmill.
- **Phase 6:** identity/capabilities, yielding/threading model, script hub UI, safety
  rails (what scripts may touch).

## Build & run

1. Open `internal/Internal.sln` in VS2022, build x64 Release.
2. Start Roblox, run `Injector.exe [path\to\UCRCore.dll]`.
3. Expect: PID + version printed, then the null-provider message. That's success
   for Phase 1 — it proves the pipeline up to the missing primitive.
4. Once a provider exists: check `%TEMP%\UCRInternal.log` for the resolve lines.
5. Demo: write a number (e.g. `100`) into `%TEMP%\UCRDemoSpeed.txt` while injected
   → WalkSpeed applies via the dual-write method. Delete the file → stays (toggle
   back with `16`).

## Rules of the war

1. Never test on an account you care about. Ever.
2. Never commit a working bypass to this repo (or anywhere public).
3. Every static address dies weekly — all RE must be pattern + runtime-validated.
4. The external stays the daily driver until internal reaches parity + safety.
