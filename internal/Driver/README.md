# UCRDrv — kernel helper (Phase 2a)

Minimal WDM driver: module base + read/write/alloc/free/protect on a target PID,
via `\\.\UCRDrv`. The injector's `DriverProvider` uses it. Stage 1 proves kernel
R/W; the DLL loader (manual map + execution) is the next stage.

## 0. Preconditions (do these first, in order)

Run in an **admin** command prompt. If any step fails, paste the exact output.

1. **Admin?** `net session` → success = admin. If "Access denied", you're not admin:
   stop here, this path needs admin.
2. **Test signing on:**
   `bcdedit /set testsigning on` → "The operation completed successfully."
   Reboot. Your desktop should show a **"Test Mode"** watermark (bottom-right).
   No watermark = it didn't apply, driver will refuse to load (error 577).
3. **Memory Integrity OFF:** Windows Security → Device security → Core isolation →
   **Memory integrity = Off**. Reboot if you changed it. (It silently blocks
   test-signed drivers when on.)
4. Still blocked? Only then: `bcdedit /set hypervisorlaunchtype off` + reboot.
   Warning: this disables Hyper-V/WSL2/Docker until you turn it back on
   (`auto`) + reboot. Most home PCs don't need this step.

## 1. Build

Requires VS2022 + **WDK** (same major version as your Windows SDK).

1. VS → Create new project → **Empty WDM Driver** (name it `UCRDrv`).
2. Add existing items: `Src/DrvMain.c`, `Src/MemOps.c`, `Src/MemOps.h`.
3. Configuration: **Release / x64**.
4. Enable test signing: project Properties → Driver Signing → General →
   **Sign Mode = Test Sign**, **Test Certificate** = create/select one.
   (Or sign manually with `signtool` + your own test cert.)
5. Build → `UCRDrv.sys`.

Why no checked-in `.vcxproj`? Wizard-generated driver projects are correct by
construction; hand-written ones break across WDK versions. Two minutes, zero risk.

## 2. Install + run

Admin prompt, in the folder with `UCRDrv.sys`:

```
sc create UCRDrv binPath= "%CD%\UCRDrv.sys" type= kernel
sc start UCRDrv
```

Expect: `STATE: 4 RUNNING`. Then run the injector: the `kernel driver (UCRDrv)`
provider should report `driver R/W OK (base=0x..., MZ ok)`.

Stop/remove later: `sc stop UCRDrv` then `sc delete UCRDrv`.

## 3. Troubleshooting

| Symptom | Cause |
|---|---|
| `sc start` → error 577 | Not test-signed, or no Test Mode watermark (step 0.2) |
| `sc start` → error 2 / 3 | Wrong `binPath` (needs the space after `binPath= `) |
| Injector: CreateFile failed (2) | Driver not running (`sc query UCRDrv`) |
| Injector: CreateFile failed (5) | Run injector as admin |
| BSOD on load/unload | Paste the bugcheck code — do not reload until fixed |
| Memory Integrity toggle is greyed | Managed by policy/antivirus; check `msinfo32` → Virtualization-based Security |

## 4. Rules (kernel is unforgiving)

- **Alts only** when this touches Roblox. Kernel + Hyperion = ban-risk territory.
- Never patch kernel structures or Hyperion's driver (PatchGuard BSOD / instant ban).
- This driver only operates on **target user-mode memory**. Keep it that way.
- Never ship the `.sys` publicly, never reuse this device name elsewhere.
- One change at a time; if it BSODs, the last change did it.
