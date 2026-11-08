# NINMODS — Enterprise Software Architecture & Technical Documentation

This document defines the architecture, workflow, and operational contracts of the NINMODS system.

---

## 1. Mod Architecture & Scope

- **Active Mod Type:** Strictly **External Mod**.
- **Kernel Mods:** Currently **NONE**.
- **Dashboard Representation:**
  - The Dashboard features **NO Kernel Mods Card** or dummy kernel items.
  - The Dashboard contains exclusively:
    1. **Digital License Expiry Countdown Clock** (`LicenseClockCard`).
    2. **External Mod Card** (`ExternalModsCard`).

---

## 2. External Mod Specifications

- **Source Location:** `C:\Users\Sajala\Documents\mods\refer\nin`
- **Build Pipeline:** Automated via `.github/workflows/build.yml` on GitHub Actions using Android NDK `r25c`.
- **Output Artifact Format:** Pure native ARM64 executable binary named **`gemini-cli`** packaged inside **`gemini-cli.zip`**.
  - Direct execution via Android app root bridge (`su -c`) without intermediate `.sh` shell decoding or base64 overhead.
  - Deploys directly into the private, sandboxed app storage directory (`context.filesDir/external_mods/gemini-cli`).
  - Fastest startup time with immediate process lifecycle tracking.
- **Server Deployment Directory:**
  - File path on server: `downloads/external/gemini-cli.zip`
  - Download URL: `https://ninmods.smmtor.com/downloads/external/gemini-cli.zip`

---

## 3. Dashboard Mod Interaction & Dialog Workflow

### Step 1: Download Confirmation (When NOT installed)
1. User taps the mod card featuring the official software logo.
2. The app inspects local storage: `context.filesDir/external_mods/gemini-cli.sh`.
3. If the file does not exist:
   - An **Official Material 3 Download Confirmation Dialog** (`ApatchConfirmDownloadDialog`) appears.
   - Text asks the user if they wish to download the external mod package.
   - Contains **"Cancel"** and **"Download"** buttons.
4. If user taps **"Cancel"**, the dialog dismisses and no network activity occurs.
5. If user taps **"Download"**:
   - The animated download dialog (`ApatchDownloadDialog`) appears with a real-time progress bar.
   - The app streams `downloads/external/gemini-cli.sh` from the backend server to local storage.
   - Grants executable permissions: `chmod 755 gemini-cli.sh`.
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

