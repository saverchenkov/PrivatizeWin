# PrivatizeWin

<div align="center">

<img src="assets/logo.png" width="160" height="160" alt="PrivatizeWin Logo" />

### PrivatizeWin
**Modern, Minimalist Windows Privacy & Telemetry Silencer**  
*A lightweight, transparent, open-source alternative to O&O ShutUp10++.*

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/Standard-C%2B%2B20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![ISO C++ Core Guidelines](https://img.shields.io/badge/Guidelines-ISO%20CppCoreGuidelines-success.svg)](https://github.com/isocpp/CppCoreGuidelines)
[![Platform: Windows 10 / 11](https://img.shields.io/badge/Platform-Windows%2010%20%7C%2011-0078D6.svg)](https://microsoft.com)
[![Footprint: < 500 KB](https://img.shields.io/badge/Binary%20Size-%3C%20500%20KB-brightgreen.svg)]()

</div>

---

## ⚡ Highlights

* **Sub-500 KB Single Executable:** Written in clean modern **C++20** with pure Win32 API. Zero runtime bloat, 0ms startup time, zero DLL dependencies.
* **Master-Detail Utility UX:** Built using native Common Controls (`SysTreeView32` + `SysListView32`) in the classic, battle-tested Sysinternals / Device Manager style. No webviews, no electron, no lag.
* **Full Modern Windows 11 & 24H2 Coverage:** Shuts down Windows Copilot, Recall snapshot recordings, dynamic MSN taskbar search widgets, forced web Outlook migration, Edge shopping trackers, and Delivery Optimization.
* **Dual GUI & Headless CLI:** Run interactively, or invoke headless via terminal and automated deployment scripts (`--apply-template`, `--dry-run`, `--status --output json`).
* **Automated Background Reapplication:** Windows Updates frequently reset privacy keys. PrivatizeWin features built-in Task Scheduler management (`--install-task daily|logon|weekly`) to silently enforce your templates in the background.
* **Configurable Multi-User Hive Targeting (`--users`):** Elevates to iterate mounted `HKEY_USERS` and dynamically mount unloaded user `NTUSER.DAT` files and the Default User template.
* **Adaptive Safety:** Automatically creates System Restore Points (`srclient.dll`) in interactive mode while avoiding VSS shadow storage exhaustion during recurring CLI task runs.
* **Strict ISO C++ Core Guidelines Compliance:** Built with RAII handle managers, zero explicit manual memory allocations, `std::string_view` parameter interfaces, and strict compiler warnings (`/W4`, `/permissive-`).

---

## 🖥️ UI Architecture (Sysinternals Style)

```
+------------------------------------------------------------------------------------+
| [File]  [Templates]  [Actions]  [Tools]  [Help]                                    |
+------------------------------------------------------------------------------------+
| [ Filter (Ctrl+F)... ] [Preset: Recommended v] [ ] Check All  [Apply] [Revert] [ ] |
+------------------------+--[||]-----------------------------------------------------+
| Categories             |  ||  [x] Privacy Tweak      | Status    | Safety  | Scope |
|------------------------+--||-------------------------+-----------+---------+-------|
| > All Settings (27)    |  ||  [x] Disable Telemetry  | Protected | Safe    | HKLM  |
|   Telemetry & Diag (7) |  ||  [x] Disable DiagTrack  | Protected | Safe    | Svc   |
|   AI & Copilot (3)     |  ||  [x] Disable Copilot    | Protected | Safe    | Both  |
|   Cortana & Search (3) |  ||  [x] Disable Recall AI  | Protected | Safe    | Both  |
|   Privacy & Tracking (4|  ||  [x] Bing in Start      | Protected | Safe    | HKCU  |
|   Office & Outlook (3) |  ||  [x] Block Web Outlook  | Protected | Safe    | HKCU  |
|   Microsoft Edge (3)   |  ||  [x] Disable Edge Svc   | Protected | Safe    | HKLM  |
|   Windows Update (2)   |  ||  [x] Disable P2P Updates| Protected | Safe    | HKLM  |
+========================+==[==]=====================================================+
| TWEAK: Disable Windows Recall Automated Screen Snapshots [AI_RECALL]               |
| CATEGORY: AI & Copilot  |  RECOMMENDATION: SAFE (Recommended for all users)         |
|                                                                                    |
| DESCRIPTION:                                                                       |
| Disables Microsoft Recall from recording screenshots of your desktop, apps, etc.  |
|                                                                                    |
| AFFECTED REGISTRY KEYS:                                                            |
|   * [HKLM\] SOFTWARE\Policies\Microsoft\Windows\WindowsCopilot -> DisableAIData=1  |
|   * [HKCU\] SOFTWARE\Microsoft\Windows\CurrentVersion\Recall -> EnableRecall=0     |
+------------------------------------------------------------------------------------+
| Privatized: 20 / 27 settings | Active Category: All Settings | Administrator       |
+------------------------------------------------------------------------------------+
```

* **Interactive Resizable Panels:** Click and drag the vertical splitter (`||`) to resize the category rail or the horizontal splitter (`==`) to expand the detail inspector.
* **Master "Check All" Header:** One-click toggle in the toolbar to check or uncheck all visible settings, with intelligent tri-state support.
* **Full Multi-Select Support:** Select multiple rows using `Ctrl` or `Shift`, press `Ctrl+A` to select all, or tap `Spacebar` to toggle check states across all highlighted rows.
* **Right-Click Context Menu:** Right-click any row to access quick actions: *Check Selected*, *Uncheck Selected*, *Invert Selection*, *Apply Selected Immediately*, *Copy Tweak ID*, and *Copy Details*.
| TWEAK: Disable Windows Recall Automated Screen Snapshots [AI_RECALL]               |
| CATEGORY: AI & Copilot  |  RECOMMENDATION: SAFE (Recommended for all users)         |
|                                                                                    |
| DESCRIPTION:                                                                       |
| Disables Microsoft Recall from recording screenshots of your desktop, apps, etc.  |
|                                                                                    |
| AFFECTED REGISTRY KEYS:                                                            |
|   * [HKLM\] SOFTWARE\Policies\Microsoft\Windows\WindowsCopilot -> DisableAIData=1  |
|   * [HKCU\] SOFTWARE\Microsoft\Windows\CurrentVersion\Recall -> EnableRecall=0     |
+------------------------------------------------------------------------------------+
| Privatized: 20 / 27 settings | Active Category: All Settings | Administrator       |
+------------------------------------------------------------------------------------+
```

---

## 🚀 CLI & Automation Reference

PrivatizeWin automatically routes to the command-line engine when arguments are passed:

```bash
# Display built-in CLI help
PrivatizeWin.exe --help

# List all available built-in presets and detected templates
PrivatizeWin.exe --list-templates

# Check current privacy status in human-readable table or machine JSON
PrivatizeWin.exe --status
PrivatizeWin.exe --status --output json

# Apply a built-in template profile
PrivatizeWin.exe --apply-template recommended
PrivatizeWin.exe --apply-template strict
PrivatizeWin.exe --apply-template minimal

# Test template changes without modifying registry or services
PrivatizeWin.exe --apply-template strict --dry-run

# Apply custom enterprise JSON template silently
PrivatizeWin.exe --apply-template C:\corp\custom_policy.json --quiet

# Revert all tweaks back to Windows factory defaults
PrivatizeWin.exe --revert
```

### Automated Reapplication (Combating Windows Updates)

Windows Feature Updates and cumulative patches often re-enable telemetry services like `DiagTrack`. PrivatizeWin can install and manage its own recurring background scheduled task:

```bash
# Schedule daily reapplication of 'recommended' profile at 12:00 PM
PrivatizeWin.exe --install-task daily --task-template recommended

# Schedule reapplication at user logon for strict privacy
PrivatizeWin.exe --install-task logon --task-template strict

# Remove the background scheduled task
PrivatizeWin.exe --uninstall-task
```

### User Hive Targeting (`--users`)

| Flag | Behavior |
| :--- | :--- |
| `--users all` *(Default)* | Iterates all mounted profiles in `HKEY_USERS` + mounts unloaded user `NTUSER.DAT` files + mounts Default User profile. |
| `--users current` | Modifies only the active user's `HKEY_CURRENT_USER`. |
| `--users none` | Skips per-user tweaks; enforces only system-wide machine policies (`HKLM`). |
| `--users alice,bob` | Targets only the specified usernames or account SIDs. |

---

## 📑 Configurable Template Format

Templates are saved as standard, human-readable JSON files:

```json
{
  "name": "strict",
  "description": "Strict privacy configuration.",
  "tweaks": {
    "TEL_DIAGTRACK": true,
    "TEL_DMWAP": true,
    "TEL_DIAGDATA": true,
    "TEL_AIT": true,
    "TEL_CEIP": true,
    "TEL_FEEDBACK": true,
    "TEL_CRASHDUMP": true,
    "AI_COPILOT": true,
    "AI_RECALL": true,
    "AI_SEARCH_HIGHLIGHTS": true,
    "SRCH_BING_START": true,
    "SRCH_CORTANA": true,
    "SRCH_CLOUD": true,
    "PRIV_AD_ID": true,
    "PRIV_TAILORED": true,
    "PRIV_KEYSTROKES": true,
    "PRIV_TIMELINE": true,
    "OUT_HIDE_TOGGLE": true,
    "OUT_BLOCK_MIGRATE": true,
    "OFFICE_TELEMETRY": true,
    "EDGE_METRICS": true,
    "EDGE_SHOPPING": true,
    "EDGE_SEARCH_SUGGEST": true,
    "WU_DELIVERY_OPT": true,
    "WU_AUTO_REBOOT": true,
    "LOCK_SPOTLIGHT_ADS": true,
    "SHELL_PROMOTED_APPS": true
  }
}
```

---

## 🛡️ ISO C++ Core Guidelines Compliance

PrivatizeWin is designed from the ground up adhering strictly to the [C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines):

* **R.1 & R.10 (RAII Handle Management):** All Win32 handles (`HANDLE`, `HKEY`, `SC_HANDLE`, `HMODULE`) are wrapped in move-only RAII smart classes (`UniqueHandle`, `UniqueHKey`, `UniqueScHandle`, `UniqueHModule`) in `src/core/SmartHandle.h`. Resource leaks are mathematically impossible.
* **R.11 (No Explicit Raw Allocation):** Zero manual calls to `new` or `delete`. The main window instance is held via `std::unique_ptr<MainWindow>`.
* **F.16 (String Views for Parameter Passing):** Read-only string parameters across all helper and registry functions use `std::string_view` and `std::wstring_view`.
* **I.10 & F.21 (Clear Interfaces):** Critical inspection and status methods are decorated with `[[nodiscard]]` and `noexcept`.
* **ES.48 & ES.49 (No C-Style Casts):** All Win32 casts strictly use `static_cast` and `reinterpret_cast`.
* **Static Analysis:** Pre-configured with `.clang-tidy` (`cppcoreguidelines-*`) and MSVC `/W4 /permissive- /utf-8`.

---

## 🛠️ Building from Source

### Prerequisites
* Windows 10 / 11 (x64 or ARM64)
* Visual Studio 2022 (or VS Build Tools) with C++20 support
* CMake 3.20+ and Ninja

### Build Instructions

```powershell
# 1. Clone repository
git clone https://github.com/saverchenkov/PrivatizeWin.git
cd PrivatizeWin

# 2. Configure build environment (MSVC x64 Developer Prompt)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# 3. Compile standalone single executable
cmake --build build --config Release

# 4. Binary output located at:
# .\build\PrivatizeWin.exe  (< 500 KB)
```

---

## 🧪 Automated Testing

PrivatizeWin includes a comprehensive automated test suite consisting of **24 Unit and Integration Tests** with zero external test dependencies:

* **Unit Tests (`tests/unit/`):**
  * `SimpleJson`: Verifies parsing of primitives, nested arrays/objects, string escaping, and round-trip serialization.
  * `SmartHandle`: Verifies move semantics, RAII cleanup, and handle transfer for `UniqueHandle`, `UniqueHKey`, and `UniqueScHandle`.
  * `CliParser`: Verifies CLI flags, scheduled task syntax, multi-user targeting (`--users`), and dry-run execution.
  * `TemplateManager`: Verifies embedded presets (`recommended`, `strict`, `minimal`, `defaults`) and JSON file export/import.
  * `TweakRegistry`: Verifies catalog integrity, category grouping, and JSON dynamic extensions.

* **Integration Tests (`tests/integration/`):**
  * `RegistryHelperLive`: Safely exercises live Windows Registry read/write/audit/delete routines within an isolated test key (`HKCU\Software\PrivatizeWin_Test_Sandbox`).
  * `ServiceHelperLive`: Verifies service discovery and startup type queries against running Windows services (`RpcSs`).
  * `UserHiveDiscovery`: Validates multi-user profile discovery and SID resolution on the active operating system.
  * `CliEndToEnd`: Spawns child `PrivatizeWin.exe` processes via Win32 pipes to assert exit codes and parse live `--status --output json` output.

### Running Tests

```powershell
# Run via CMake CTest
ctest --test-dir build --output-on-failure

# Or execute the test binary directly for detailed test reports:
.\build\tests\PrivatizeWin_Tests.exe
```

---

## 📄 License

PrivatizeWin is licensed under the [MIT License](LICENSE).
