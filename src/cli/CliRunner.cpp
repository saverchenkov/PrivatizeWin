#include "CliRunner.h"
#include "../core/TweakRegistry.h"
#include "../core/TemplateManager.h"
#include "../core/RestorePoint.h"
#include "../core/TaskScheduler.h"
#include "../core/SimpleJson.h"
#include "../core/ProcessHelper.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <io.h>
#include <fcntl.h>

namespace PrivatizeWin {

static std::string WStringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    const int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) return std::string();
    std::string result(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], size, nullptr, nullptr);
    return result;
}

static void InitializeConsoleOutput() {
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdOut == nullptr || hStdOut == INVALID_HANDLE_VALUE) {
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            FILE* fpOut = nullptr;
            FILE* fpErr = nullptr;
            freopen_s(&fpOut, "CONOUT$", "w", stdout);
            freopen_s(&fpErr, "CONOUT$", "w", stderr);
            std::cout.clear();
            std::cerr.clear();
        }
    }
}

CliOptions CliRunner::ParseArguments(int argc, wchar_t* argv[]) {
    CliOptions opts;
    if (argc <= 1) {
        opts.isCli = false;
        return opts;
    }

    for (int i = 1; i < argc; ++i) {
        const std::wstring arg = argv[i];

        if (arg == L"--help" || arg == L"-h" || arg == L"/?") {
            InitializeConsoleOutput();
            PrintHelp();
            exit(0);
        } else if (arg == L"--list-templates") {
            opts.listTemplates = true;
        } else if (arg == L"--apply-template") {
            if (i + 1 < argc && argv[i + 1][0] != L'-') {
                opts.applyTemplate = WStringToUtf8(argv[++i]);
            } else {
                opts.hasError = true;
                opts.errorMessage = "Missing template name or path for --apply-template";
            }
        } else if (arg == L"--revert") {
            opts.revertAll = true;
        } else if (arg == L"--status") {
            opts.showStatus = true;
        } else if (arg == L"--output") {
            if (i + 1 < argc && argv[i + 1][0] != L'-') {
                opts.statusFormat = WStringToUtf8(argv[++i]);
            } else {
                opts.hasError = true;
                opts.errorMessage = "Missing format (text|json) for --output";
            }
        } else if (arg == L"--install-task") {
            opts.installTask = true;
            if (i + 1 < argc && argv[i + 1][0] != L'-') {
                opts.taskFrequency = WStringToUtf8(argv[++i]);
            }
        } else if (arg == L"--task-template") {
            if (i + 1 < argc && argv[i + 1][0] != L'-') {
                opts.taskTemplate = WStringToUtf8(argv[++i]);
            } else {
                opts.hasError = true;
                opts.errorMessage = "Missing template name for --task-template";
            }
        } else if (arg == L"--uninstall-task") {
            opts.uninstallTask = true;
        } else if (arg == L"--dry-run") {
            opts.dryRun = true;
        } else if (arg == L"--quiet" || arg == L"-q" || arg == L"--silent") {
            opts.quiet = true;
        } else if (arg == L"--create-restore-point") {
            opts.forceRestorePoint = true;
        } else if (arg == L"--no-restore-point") {
            opts.noRestorePoint = true;
        } else if (arg == L"--users") {
            if (i + 1 < argc && argv[i + 1][0] != L'-') {
                const std::wstring val = argv[++i];
                if (val == L"all") {
                    opts.userMode = UserSelectionMode::AllUsers;
                } else if (val == L"current") {
                    opts.userMode = UserSelectionMode::CurrentUser;
                } else if (val == L"none") {
                    opts.userMode = UserSelectionMode::NoUsers;
                } else {
                    opts.userMode = UserSelectionMode::SpecificUsers;
                    std::wstringstream wss(val);
                    std::wstring item;
                    while (std::getline(wss, item, L',')) {
                        if (!item.empty()) opts.specificUsernames.push_back(item);
                    }
                }
            } else {
                opts.hasError = true;
                opts.errorMessage = "Missing value (all|current|none|user1,user2) for --users";
            }
        } else if (arg == L"--no-elevate") {
            // Internal flag for non-elevated relaunch
        } else if (arg == L"--resume-pending") {
            if (i + 1 < argc && argv[i + 1][0] != L'-') {
                opts.resumePendingFile = argv[++i];
            } else {
                opts.hasError = true;
                opts.errorMessage = "Missing file path for --resume-pending";
            }
        } else {
            opts.hasError = true;
            opts.errorMessage = "Unrecognized command-line option: " + WStringToUtf8(arg);
        }
    }

    if (opts.hasError) {
        opts.isCli = true;
        return opts;
    }

    opts.isCli = false;
    for (int j = 1; j < argc; ++j) {
        if (wcscmp(argv[j], L"--no-elevate") == 0) {
            continue;
        }
        if (wcscmp(argv[j], L"--resume-pending") == 0) {
            if (j + 1 < argc && argv[j + 1][0] != L'-') {
                j++;
            }
            continue;
        }
        opts.isCli = true;
        break;
    }

    return opts;
}

int CliRunner::Execute(const CliOptions& opts) {
    if (!opts.quiet) {
        InitializeConsoleOutput();
    }

    if (opts.hasError) {
        InitializeConsoleOutput();
        std::cerr << "[ERROR] " << opts.errorMessage << "\n"
                  << "        Run 'PrivatizeWin.exe --help' for usage.\n" << std::flush;
        return 1;
    }

    TweakRegistry::Instance().InitializeDefaultTweaks();
    TemplateManager::Instance().InitializeBuiltinTemplates();

    // 1. List Templates
    if (opts.listTemplates) {
        if (!opts.quiet) {
            std::cout << "\n=======================================================\n";
            std::cout << "  PrivatizeWin Available Templates\n";
            std::cout << "=======================================================\n";
            const auto names = TemplateManager::Instance().GetAvailableTemplateNames();
            for (const auto& name : names) {
                const auto t = TemplateManager::Instance().GetTemplate(name);
                std::cout << "  * " << std::left << std::setw(15) << name;
                if (t.has_value()) {
                    std::cout << " - " << t->description;
                }
                std::cout << "\n";
            }
            std::cout << "\nCustom templates (.json) can also be passed via path:\n";
            std::cout << "  PrivatizeWin.exe --apply-template C:\\path\\to\\custom.json\n\n" << std::flush;
        }
        return 0;
    }

    // 2. Install Scheduled Task
    if (opts.installTask) {
        if (opts.dryRun) {
            if (!opts.quiet) {
                std::cout << "[DRY RUN] Would install scheduled task 'PrivatizeWin Auto-Protect'\n"
                          << "          Frequency: " << opts.taskFrequency << "\n"
                          << "          Template:  " << opts.taskTemplate << "\n"
                          << "          Users:     " << (opts.userMode == UserSelectionMode::AllUsers ? "all" : (opts.userMode == UserSelectionMode::CurrentUser ? "current" : "none")) << "\n" << std::flush;
            }
            return 0;
        }

        if (!IsRunningAsAdmin()) {
            InitializeConsoleOutput();
            std::cerr << "[ERROR] Administrator privileges are required to configure scheduled tasks.\n"
                      << "        Please run Command Prompt or PowerShell as Administrator.\n" << std::flush;
            return 5;
        }

        ScheduledTaskConfig taskConfig;
        taskConfig.frequency = std::wstring(opts.taskFrequency.begin(), opts.taskFrequency.end());
        taskConfig.templateName = std::wstring(opts.taskTemplate.begin(), opts.taskTemplate.end());

        if (opts.userMode == UserSelectionMode::AllUsers) taskConfig.userMode = L"all";
        else if (opts.userMode == UserSelectionMode::CurrentUser) taskConfig.userMode = L"current";
        else taskConfig.userMode = L"none";

        const bool ok = TaskScheduler::InstallTask(taskConfig);
        if (!opts.quiet) {
            if (ok) {
                std::cout << "[SUCCESS] Scheduled task 'PrivatizeWin Auto-Protect' installed successfully.\n";
                std::cout << "          Frequency: " << opts.taskFrequency << "\n";
                std::cout << "          Template:  " << opts.taskTemplate << "\n";
                std::cout << "          Users:     " << WStringToUtf8(taskConfig.userMode) << "\n" << std::flush;
            } else {
                std::cerr << "[ERROR] Failed to install scheduled task. Ensure you are running as Administrator.\n" << std::flush;
            }
        }
        return ok ? 0 : 1;
    }

    // 3. Uninstall Scheduled Task
    if (opts.uninstallTask) {
        if (opts.dryRun) {
            if (!opts.quiet) {
                std::cout << "[DRY RUN] Would remove scheduled task 'PrivatizeWin Auto-Protect'\n" << std::flush;
            }
            return 0;
        }

        if (!IsRunningAsAdmin()) {
            InitializeConsoleOutput();
            std::cerr << "[ERROR] Administrator privileges are required to remove scheduled tasks.\n"
                      << "        Please run Command Prompt or PowerShell as Administrator.\n" << std::flush;
            return 5;
        }

        const bool ok = TaskScheduler::UninstallTask();
        if (!opts.quiet) {
            if (ok) {
                std::cout << "[SUCCESS] Scheduled task 'PrivatizeWin Auto-Protect' removed.\n" << std::flush;
            } else {
                std::cerr << "[ERROR] Failed to remove scheduled task or task was not installed.\n" << std::flush;
            }
        }
        return ok ? 0 : 1;
    }

    // 4. Show Status
    if (opts.showStatus) {
        const auto& tweaks = TweakRegistry::Instance().GetAllTweaks();
        if (opts.statusFormat == "json") {
            JsonValue root(JsonType::Object);
            JsonValue items(JsonType::Array);
            for (const auto& t : tweaks) {
                const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, opts.userMode, opts.specificUsernames);
                JsonValue obj(JsonType::Object);
                obj["id"] = t.id;
                switch (st) {
                case SettingStatus::Applied: obj["status"] = "applied"; break;
                case SettingStatus::NotApplied: obj["status"] = "not_applied"; break;
                case SettingStatus::Partial: obj["status"] = "partial"; break;
                case SettingStatus::Unknown: obj["status"] = "unknown"; break;
                case SettingStatus::NotApplicable: obj["status"] = "not_applicable"; break;
                }
                items.arrayValue.push_back(std::move(obj));
            }
            root["tweaks"] = std::move(items);
            std::cout << root.toString(2) << "\n" << std::flush;
        } else {
            std::cout << "\n" << std::left << std::setw(25) << "SETTING ID" << std::setw(16) << "STATUS" << "TITLE\n";
            std::cout << std::string(75, '-') << "\n";
            for (const auto& t : tweaks) {
                const SettingStatus st = TweakRegistry::Instance().AuditTweak(t.id, opts.userMode, opts.specificUsernames);
                std::cout << std::left << std::setw(25) << t.id;
                switch (st) {
                case SettingStatus::Applied: std::cout << std::setw(16) << "[APPLIED]"; break;
                case SettingStatus::NotApplied: std::cout << std::setw(16) << "[NOT APPLIED]"; break;
                case SettingStatus::Partial: std::cout << std::setw(16) << "[PARTIAL]"; break;
                case SettingStatus::Unknown: std::cout << std::setw(16) << "[UNKNOWN]"; break;
                case SettingStatus::NotApplicable: std::cout << std::setw(16) << "[NOT APPLICABLE]"; break;
                }

                std::cout << WStringToUtf8(t.title) << "\n";
            }
            std::cout << "\n" << std::flush;
        }
        return 0;
    }

    // 5. Apply Template or Revert
    std::string templateToApply = opts.applyTemplate;
    if (opts.revertAll) {
        templateToApply = "defaults";
    }

    if (!templateToApply.empty()) {
        if (!opts.dryRun && !IsRunningAsAdmin()) {
            InitializeConsoleOutput();
            std::cerr << "[ERROR] Administrator privileges are required to modify system privacy settings.\n"
                      << "        Please run Command Prompt or PowerShell as Administrator.\n" << std::flush;
            return 5;
        }

        TemplateProfile profile;
        const auto builtin = TemplateManager::Instance().GetTemplate(templateToApply);
        if (builtin.has_value()) {
            profile = builtin.value();
        } else {
            const std::wstring wpath(templateToApply.begin(), templateToApply.end());
            std::string loadErr;
            if (!TemplateManager::Instance().LoadTemplateFromFile(wpath, profile, &loadErr)) {
                InitializeConsoleOutput();
                std::cerr << "[ERROR] Failed to load template '" << templateToApply << "': " << loadErr << "\n" << std::flush;
                return 3;
            }
        }

        // Adaptive Safety Restore Point
        if (opts.forceRestorePoint) {
            if (opts.dryRun) {
                if (!opts.quiet) std::cout << "[DRY RUN] Would create System Restore Point: 'PrivatizeWin - Pre-Apply Configuration'\n" << std::flush;
            } else {
                int64_t seq = 0;
                if (!opts.quiet) std::cout << "[*] Creating System Restore Point...\n" << std::flush;
                const bool rpOk = RestorePoint::Create(L"PrivatizeWin - Pre-Apply Configuration", seq);
                if (!opts.quiet) {
                    if (rpOk) std::cout << "[+] System Restore Point created successfully.\n" << std::flush;
                    else std::cout << "[!] Warning: Could not create restore point (System Protection may be disabled).\n" << std::flush;
                }
            }
        }

        if (!opts.quiet) {
            std::cout << "[*] " << (opts.dryRun ? "[DRY RUN] " : "") << "Applying template: " << profile.name << "\n";
            if (!profile.description.empty()) {
                std::cout << "    Description: " << profile.description << "\n";
            }
            std::cout << std::flush;
        }

        int appliedCount = 0;
        int failedCount = 0;
        int skippedCount = 0;
        for (const auto& [tweakId, shouldEnable] : profile.tweakStates) {
            const Tweak* t = TweakRegistry::Instance().GetTweakById(tweakId);
            if (!t) {
                skippedCount++;
                if (!opts.quiet) {
                    std::cerr << "    [!] Warning: Unknown tweak ID '" << tweakId << "'\n" << std::flush;
                }
                continue;
            }

            if (opts.dryRun) {
                if (!opts.quiet) {
                    std::cout << "    -> Would set " << tweakId << " to " << (shouldEnable ? "PROTECTED" : "DEFAULT") << "\n" << std::flush;
                }
            } else {
                const bool ok = TweakRegistry::Instance().ApplyTweak(tweakId, shouldEnable, opts.userMode, opts.specificUsernames);
                if (ok) {
                    appliedCount++;
                } else {
                    failedCount++;
                    InitializeConsoleOutput();
                    std::cerr << "    [!] Failed to configure " << tweakId << "\n" << std::flush;
                }
            }
        }

        if (opts.dryRun) {
            if (!opts.quiet) {
                std::cout << "[SUCCESS] Dry run completed without making any changes. (" << profile.tweakStates.size() << " operations planned)\n" << std::flush;
            }
            return 0;
        }

        if (!opts.quiet) {
            std::cout << "[SUMMARY] Applied: " << appliedCount << ", Failed: " << failedCount;
            if (skippedCount > 0) std::cout << ", Skipped: " << skippedCount;
            std::cout << "\n" << std::flush;
        }

        if (failedCount > 0) {
            InitializeConsoleOutput();
            std::cerr << "[ERROR] One or more tweaks failed to apply.\n" << std::flush;
            return 1;
        }
        return 0;
    }

    // No valid CLI command was passed
    InitializeConsoleOutput();
    std::cerr << "[ERROR] No command specified.\n"
              << "        Run 'PrivatizeWin.exe --help' for usage.\n" << std::flush;
    return 1;
}

void CliRunner::PrintHelp() noexcept {
    std::cout << R"(
PrivatizeWin - Modern, Minimalist Windows Privacy & Telemetry Silencer
Copyright (C) 2026 PrivatizeWin Project (MIT License)

USAGE:
  PrivatizeWin.exe [COMMAND / OPTIONS]

COMMANDS:
  --apply-template <name|path.json>  Apply a privacy template profile
  --list-templates                   List all built-in and detected templates
  --status [--output text|json]      Display audit status of all privacy tweaks
  --revert                           Revert all settings back to Windows defaults

AUTOMATION & SCHEDULED TASKS:
  --install-task [daily|logon|weekly] Install recurring background task
                                      (Default: daily at 12:00)
  --task-template <name>              Template for scheduled task (Default: recommended)
  --uninstall-task                    Remove the background scheduled task

USER HIVE TARGETING (--users):
  --users all                        Apply per-user settings to ALL profiles (Default)
  --users current                    Apply only to current interactive user session
  --users none                       Skip HKCU tweaks; apply only machine-wide HKLM
  --users user1,user2                Target specific usernames or SIDs

SAFETY & EXECUTION FLAGS:
  --dry-run                          Simulate changes without modifying system
  --quiet, -q, --silent              Run silently with no console output or popups
  --create-restore-point             Force creation of a System Restore Point
  --no-restore-point                 Skip System Restore Point creation

EXAMPLES:
  # One-click apply recommended safe privacy settings
  PrivatizeWin.exe --apply-template recommended

  # Schedule daily reapplication of strict privacy in background
  PrivatizeWin.exe --install-task daily --task-template strict

  # Check current machine privacy audit in JSON format
  PrivatizeWin.exe --status --output json

  # Silently apply custom enterprise policy to all users
  PrivatizeWin.exe --apply-template C:\corp\hardened.json --users all --quiet
)" << std::flush;
}

} // namespace PrivatizeWin
