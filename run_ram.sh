#!/bin/sh
# ═══════════════════════════════════════════════════════════════════════════════
# Google Gemini Developer API CLI — RAM / Memory Loader (Fileless Execution)
# ═══════════════════════════════════════════════════════════════════════════════

BASE_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN_NAME="gemini-cli"
SRC_BIN="$BASE_DIR/$BIN_NAME"

# Fallback: check libs path if built via NDK
if [ ! -f "$SRC_BIN" ]; then
    if [ -f "$BASE_DIR/libs/arm64-v8a/$BIN_NAME" ]; then
        SRC_BIN="$BASE_DIR/libs/arm64-v8a/$BIN_NAME"
    elif [ -f "./$BIN_NAME" ]; then
        SRC_BIN="./$BIN_NAME"
    fi
fi

if [ ! -f "$SRC_BIN" ]; then
    echo "[-] Binary $BIN_NAME not found. Please compile or download first."
    exit 1
fi

# Ensure root execution for memory & display access
if [ "$(id -u)" -ne 0 ]; then
    echo "[*] Requesting superuser privileges..."
    exec su -c "sh \"$0\" \"$@\""
fi

# ═══════════════════════════════════════════════════════════════════════════════
# RAM EXECUTION ENGINE
# ═══════════════════════════════════════════════════════════════════════════════
# Check available memory-backed temporary filesystems in RAM
RAM_DIR=""
if [ -d "/dev/shm" ] && [ -w "/dev/shm" ]; then
    RAM_DIR="/dev/shm"
elif [ -d "/dev" ] && [ -w "/dev" ]; then
    RAM_DIR="/dev"
elif [ -d "/data/adb" ] && [ -w "/data/adb" ]; then
    RAM_DIR="/data/adb"
else
    RAM_DIR="/dev"
fi

RUN_ID="$(date +%s)_$RANDOM"
MEM_BIN="$RAM_DIR/.gemini_proc_$RUN_ID"

# 1. Copy binary directly into RAM-backed buffer
cp -f "$SRC_BIN" "$MEM_BIN" 2>/dev/null
chmod 755 "$MEM_BIN" 2>/dev/null

# 2. Spawn process into memory
# Once running, the binary internally masks its name to "gemini-cli" via prctl and overwrites cmdline
"$MEM_BIN" "$@" &
PID=$!

# 3. Fileless Disarm: Immediately unlink the temporary entry from filesystem while the open file descriptor runs strictly in RAM
sleep 0.4
rm -f "$MEM_BIN" 2>/dev/null

# Wait for process lifecycle
wait $PID 2>/dev/null
