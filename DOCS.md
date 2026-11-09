# NINMODS — Enterprise Software Architecture & Technical Documentation

This document defines the architecture, workflow, and operational contracts of the NINMODS system.

---

## 1. Mod Architecture & Scope

- **Supported Mod Engines:**
  1. **NINMODS Kernel Mod** (`ninmods-kernel`): Native kernel-level overlay engine interacting with kernel drivers.
  2. **NINMODS External Mod** (`gemini-cli`): Standalone native memory overlay engine.
- **Execution Architecture:**
  - Both mod engines compile directly into pure native ARM64 ELF binaries.
  - Zero intermediate shell wrappers (`.sh` files eliminated).
  - Executed directly via superuser bridge (`su -c`) with standard process lifecycle management.
- **Dashboard Representation:**
  - The Dashboard features:
    1. **Digital License Expiry Countdown Clock** (`LicenseClockCard`).
    2. **Kernel Mod Card** (`KernelModsCard`) styled in Cyber Purple (`#AD47FA`).
    3. **External Mod Card** (`ExternalModsCard`) styled in Cyan Blue (`#00F2FE`).

---

## 2. Dedicated Server Download Specifications (Strict Subfolders)

All mod packages and drivers MUST be downloaded exclusively from the backend server (`https://ninmods.smmtor.com`).

### 2.1 Production Download URL Matrix
- **External Mod:** `https://ninmods.smmtor.com/downloads/external/gemini-cli.zip`
  - Local path: `app_backend/downloads/external/gemini-cli.zip`
  - Content: `gemini-cli` (pure native ARM64 ELF executable)
- **Kernel Mod:** `https://ninmods.smmtor.com/downloads/kernel/ninmods-kernel.zip`
  - Local path: `app_backend/downloads/kernel/ninmods-kernel.zip`
  - Content: `ninmods-kernel` (pure native ARM64 ELF executable)
- **Kernel Drivers:** `https://ninmods.smmtor.com/downloads/Kernel_Drivers.zip`
  - Local path: `app_backend/downloads/Kernel_Drivers.zip`
  - Content: Compiled `.ko` kernel modules and driver assets
- **App Release APK:** `https://ninmods.smmtor.com/downloads/app-release.apk`
  - Local path: `app_backend/downloads/app-release.apk`
  - Latest production release APK (e.g. v1.4.9)

### 2.2 Prohibited Actions (Never Deviate)
- **NO GITHUB DOWNLOAD LINKS:** Never use GitHub releases or raw GitHub URLs for mod/driver downloads in the app.
- **NO BUNDLED ASSETS IN APK:** Mod packages must never be bundled into the APK `assets/` folder. All mods are downloaded on demand.
- **NO FLAT ROOT DOWNLOAD PATHS:** Never store or request mod packages at the root of downloads (e.g. `/downloads/gemini-cli.zip` or `/downloads/ninmods-kernel.zip` are strictly FORBIDDEN). Subdirectories (`external/` and `kernel/`) are mandatory.

---

## 3. Dashboard Mod Interaction & Dialog Workflow

### Step 1: Download Confirmation (When NOT installed)
1. User taps the mod card or the CloudDownload icon button on Dashboard.
2. The app inspects local storage (`context.filesDir/external_mods/gemini-cli`).
3. If missing:
   - An **Official Material 3 Download Confirmation Dialog** (`ApatchConfirmDownloadDialog`) appears.
   - Text asks the user if they wish to download the external mod package.
   - Contains **"Cancel"** and **"Download"** buttons.
4. If user taps **"Cancel"**, the dialog dismisses and no network activity occurs.
5. If user taps **"Download"**:
   - The animated download dialog (`ApatchDownloadDialog`) appears with a real-time progress bar.
   - The app streams `downloads/external/gemini-cli.zip` from the backend server to local storage.
   - Grants executable permissions: `chmod 755 gemini-cli`.
   - Card status changes to `READY`.

### Step 2: Single-Tap Launch & Background Prompt
1. User taps the mod card when `gemini-cli.sh` exists locally.
2. The app immediately initiates background execution in memory using root shell (`InteractiveProcessBridge`) without redundant intermediate confirmation screens.
3. The native binary starts in memory and outputs the stdin authentication prompt:
   ```
   [?] Enter API Access Key:
   ```
4. `InteractiveProcessBridge` detects this stdout sequence and triggers `onLicensePrompt()`.

### Step 3: License Key Pop-up Dialog & Floating Logo Activation
1. The app automatically surfaces the **Material 3 License Key Pop-up Dialog** (`ApatchLicenseKeyDialog`) — the single interactive dialog:
   - Input field: Pre-filled with `sessionManager.activeLicenseKey` or account credential.
   - Action buttons: **"Cancel"** and **"Verify"**.
2. If user taps **"Cancel"**, the background process is terminated cleanly.
3. When user taps **"Verify"**:
   - The button shows an in-button spinner (`CircularProgressIndicator`) and label `"Verifying..."`.
   - The app verifies with backend (`/api/v2/app/activate`) and pipes the key to the running process stdin (`processBridge.sendInput(key)`).
   - The native binary receives the key, verifies online, and logs:
     `[+] API Key validated successfully. Core engine authorized.`
   - The native surface engine (`ANativeWindowCreator` & ImGui) initializes and displays the **Official Floating Logo** directly over the screen!
   - The License Dialog dismisses automatically and the card status updates to `ACTIVE`.
4. If user taps the card while `ACTIVE`, an official confirmation dialog offers to **"Stop Mod"** and unload from memory.

---

## 4. Live License Clock Policy

1. **Card Visibility:**
   - The digital license countdown clock card (`LicenseClockCard`) is strictly shown **ONLY** when a paid subscription/license plan exists and has remaining seconds > 0.
   - If no plan is active or the timer expires, the entire card is omitted from the UI.
2. **Real-Time Digital Countdown:**
   - The clock updates live every 1 second in `HH : MM : SS` digital format without requiring manual page refresh.
   - Backed by monotonic Android system uptime (`SystemClock.elapsedRealtime()`) synchronized with the backend heartbeat endpoint (`/api/v2/app/heartbeat`).

---

## 5. Directory & File Matrix

```text
DriverLoader/
├── app/src/main/java/com/andorid/ninmods/
│   ├── data/
│   │   ├── InteractiveProcessBridge.kt     # Interactive stdin/stdout root shell bridge
│   │   ├── ModDownloader.kt                # HTTP streaming file downloader
│   │   └── SessionManager.kt               # Monotonic license duration & credentials
│   ├── ui/
│   │   ├── dashboard/
│   │   │   ├── DashboardScreen.kt          # Renders LicenseClockCard & ExternalModsCard
│   │   │   └── external_mods/
│   │   │       └── ExternalModsCard.kt     # Single official external mod item
│   │   ├── clock/
│   │   │   └── LicenseClockCard.kt         # Live ticking digital countdown clock
│   │   └── dialogs/
│   │       └── ModActionDialogs.kt         # Material 3 confirm, animated download & license dialogs
│
app_backend/
├── downloads/
│   ├── external/gemini-cli.sh              # Target download for external mod
│   └── kernel/                             # Reserved for future kernel mods
│
refer/nin/
├── .github/workflows/build.yml             # Builds & packages gemini-cli.sh
├── package_sh.sh                           # Self-extracting runner packaging script
├── src/main.cpp                            # Entry point & stdin prompt gate
└── src/protect/license.cpp                 # Online HTTP license verification
```

---

## 6. Official GitHub Release & Home Page Policy

1. **GitHub Home Page "Latest" Badge Invariant:**
   - Every production release (`v1.x.x`) MUST be explicitly pinned as the repository's **"Latest Release"** on the GitHub homepage sidebar (`/releases/latest`).
   - Old versions (such as `v1.1.4`) must NEVER remain or revert to the "Latest" release on the repository front page.
2. **Automated Pipeline Enforcement (`.github/workflows/build.yml`):**
   - The release job only triggers on semantic version tags (`tags: [ 'v*' ]`) or resolves the exact `versionName` from `app/build.gradle.kts`.
   - The publishing step strictly enforces `makeLatest: true` so GitHub's releases index immediately displays the newest build with the green "Latest" badge.
3. **Artifact Consistency Guarantee:**
   - Every official release artifact (`app-release.apk`) must be mirrored simultaneously across:
     1. GitHub Releases download URL: `https://github.com/mohammadnillislam11-ai/Kernel-Driver/releases/download/<TAG>/app-release.apk`
     2. Server Backend Mirror: `app_backend/downloads/app-release.apk`
     3. Bundled Asset Fallback: `DriverLoader/app/src/main/assets/external/gemini-cli.zip`

---

## 7. Zero Local Build Policy — Strict GitHub Actions CI/CD Only

> [!IMPORTANT]
> **MANDATORY INVARIANT: NO LOCAL MACHINE COMPILATION**
> - **আমাদের লোকাল কম্পিউটার দিয়ে কোনো কিছু বিল্ড করা যাবে না (Zero Local Builds):**
>   - লোকাল পিসি/ওয়ার্কস্টেশনে NDK, CMake, বা Clang দিয়ে কোনো নেটিভ বাইনারি বা রিলিজ বিল্ড চালানো সম্পূর্ণভাবে নিষিদ্ধ ও অপ্রয়োজনীয়।
>   - লোকাল কম্পিউটারে কোনো কম্পাইলার টুলচেইন ইনস্টল বা কনফিগার করার প্রয়োজন নেই।
> - **বিল্ড সবসময় ১০০% GitHub Actions ক্লাউড CI/CD দিয়ে হবে (Always Build on GitHub):**
>   - সবধরনের কম্পাইলেশন, অপ্টিমাইজেশন (LLVM strip), প্যাকেজিং এবং আর্টফ্যাক্ট তৈরি সম্পূর্ণ অটোমেটেডভাবে GitHub Actions-এর ক্লিন ভার্চুয়াল উবুন্টু রানারে (`ubuntu-latest` + NDK `r25c`) সম্পন্ন হবে।
> - **স্ট্যান্ডার্ড ওয়ার্কফ্লো (Standard Workflow):**
>   1. কোড এডিট বা চেঞ্জ করার পর লোকালি গিট কমিট করে সরাসরি রিমোট `main` ব্রাঞ্চে `git push` করতে হবে।
>   2. পুশ করার সাথে সাথেই `.github/workflows/build.yml` অটো ট্রিগার হবে এবং ক্লাউডে ফ্রেশ আর্টফ্যাক্ট (`gemini-cli.zip`) বিল্ড করে আউটপুট তৈরি করবে।
>   3. বিল্ড শেষে গিটহাব অ্যাকশন্স আর্টফ্যাক্ট বা রিলিজ থেকে রেডি বাইনারি ডাউনলোড করে সার্ভারে (`app_backend/downloads/external/gemini-cli.zip`) স্থাপন করা হবে।


