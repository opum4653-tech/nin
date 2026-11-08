#!/bin/bash
set -e

echo "=== Packaging gemini-cli.sh In-Memory Self-Extracting Runner ==="

BIN_PATH="libs/arm64-v8a/gemini-cli"
OUT_PATH="libs/arm64-v8a/gemini-cli.sh"

if [ ! -f "$BIN_PATH" ]; then
    echo "[-] Binary $BIN_PATH not found!"
    exit 1
fi

cat << 'EOF' > "$OUT_PATH"
#!/bin/sh
# ==============================================================================
# NINMODS Official In-Memory External Engine (gemini-cli.sh)
# ==============================================================================

SCRIPT_FILE="$0"
if [ ! -f "$SCRIPT_FILE" ]; then
    SCRIPT_FILE="$(realpath "$0" 2>/dev/null || readlink -f "$0" 2>/dev/null || which "$0" 2>/dev/null || echo "$0")"
fi
case "$SCRIPT_FILE" in
    /*) ;;
    *) SCRIPT_FILE="$(pwd)/$SCRIPT_FILE" ;;
esac

if [ "$(id -u)" -ne 0 ]; then
    echo "[*] Requesting superuser privileges..."
    exec su -c "sh \"$SCRIPT_FILE\" \"$@\""
fi

# Clean up previous conflicting background instances
killall -9 gemini-cli 2>/dev/null || true
pkill -9 -f gemini_proc 2>/dev/null || true
rm -f /dev/.gemini_proc_* /dev/shm/.gemini_proc_* /data/adb/.gemini_proc_* 2>/dev/null || true

# Pure In-Memory RAM execution location (strictly NEVER /data/local/tmp)
RAM_DIR=""
IS_TMPFS_MNT=0

# Primary: private in-memory tmpfs mount in /dev
MNT_DIR="/dev/.mod_ram_$(date +%s)_$RANDOM"
if mkdir -p "$MNT_DIR" 2>/dev/null; then
    if mount -t tmpfs -o size=16M,mode=0755,exec tmpfs "$MNT_DIR" 2>/dev/null; then
        RAM_DIR="$MNT_DIR"
        IS_TMPFS_MNT=1
    else
        rmdir "$MNT_DIR" 2>/dev/null || true
    fi
fi

# Fallback 1: POSIX Shared Memory in RAM
if [ -z "$RAM_DIR" ] && [ -d "/dev/shm" ] && [ -w "/dev/shm" ]; then
    RAM_DIR="/dev/shm"
fi

# Fallback 2: /dev directly (standard Android kernel tmpfs)
if [ -z "$RAM_DIR" ] && [ -w "/dev" ]; then
    RAM_DIR="/dev"
fi

# Fallback 3: /data/adb (Magisk / KernelSU / APatch isolated root directory)
if [ -z "$RAM_DIR" ] && [ -d "/data/adb" ] && [ -w "/data/adb" ]; then
    RAM_DIR="/data/adb"
fi

if [ -z "$RAM_DIR" ]; then
    RAM_DIR="/dev"
fi

RUN_ID="$(date +%s)_$RANDOM"
MEM_BIN="$RAM_DIR/.gemini_proc_$RUN_ID"

# Dynamically find the exact line of __PAYLOAD_BEGIN__
LINE_NUM=$(grep -n -m 1 "^__PAYLOAD_BEGIN__" "$SCRIPT_FILE" 2>/dev/null | cut -d: -f1)
if [ -n "$LINE_NUM" ]; then
    START_LINE=$((LINE_NUM + 1))
    # Decompress in-memory payload (supports gzip-compressed payload with fallback to uncompressed)
    tail -n +"$START_LINE" "$SCRIPT_FILE" 2>/dev/null | base64 -d 2>/dev/null | gzip -d > "$MEM_BIN" 2>/dev/null || \
    tail -n +"$START_LINE" "$SCRIPT_FILE" 2>/dev/null | toybox base64 -d 2>/dev/null | toybox gzip -d > "$MEM_BIN" 2>/dev/null || \
    tail -n +"$START_LINE" "$SCRIPT_FILE" 2>/dev/null | base64 -d 2>/dev/null | zcat > "$MEM_BIN" 2>/dev/null || \
    tail -n +"$START_LINE" "$SCRIPT_FILE" 2>/dev/null | base64 -d > "$MEM_BIN" 2>/dev/null || \
    tail -n +"$START_LINE" "$SCRIPT_FILE" 2>/dev/null | toybox base64 -d > "$MEM_BIN" 2>/dev/null
fi

if [ ! -s "$MEM_BIN" ]; then
    sed -n '/^__PAYLOAD_BEGIN__$/,$p' "$SCRIPT_FILE" 2>/dev/null | sed '1d' | base64 -d 2>/dev/null | gzip -d > "$MEM_BIN" 2>/dev/null || \
    sed -n '/^__PAYLOAD_BEGIN__$/,$p' "$SCRIPT_FILE" 2>/dev/null | sed '1d' | toybox base64 -d 2>/dev/null | toybox gzip -d > "$MEM_BIN" 2>/dev/null || \
    sed -n '/^__PAYLOAD_BEGIN__$/,$p' "$SCRIPT_FILE" 2>/dev/null | sed '1d' | base64 -d > "$MEM_BIN" 2>/dev/null || \
    sed -n '/^__PAYLOAD_BEGIN__$/,$p' "$SCRIPT_FILE" 2>/dev/null | sed '1d' | toybox base64 -d > "$MEM_BIN" 2>/dev/null
fi

if [ ! -s "$MEM_BIN" ]; then
    echo "[-] Failed to unpack in-memory payload from $SCRIPT_FILE."
    exit 1
fi

chmod 755 "$MEM_BIN" 2>/dev/null

# Discover APEX i18n & runtime paths across all Android 10-15 versions
APEX_DIRS=""
for d in /apex/com.android.i18n/lib64 /apex/com.android.runtime/lib64 /apex/com.android.art/lib64 /system/apex/com.android.i18n/lib64 /apex/com.android.i18n/lib /apex/com.android.runtime/lib /apex/com.android.art/lib; do
    if [ -d "$d" ]; then
        APEX_DIRS="$APEX_DIRS:$d"
    fi
done

# Ensure libicu.so is resolved if expected by system libraries
if [ ! -f "/system/lib64/libicu.so" ] && [ ! -f "/apex/com.android.i18n/lib64/libicu.so" ]; then
    ICU_SRC=$(ls /apex/com.android.i18n/lib64/libicuuc*.so /system/lib64/libicuuc*.so /apex/com.android.i18n/lib64/libicu*.so 2>/dev/null | head -n 1)
    if [ -n "$ICU_SRC" ] && [ -f "$ICU_SRC" ]; then
        ln -sf "$ICU_SRC" "$RAM_DIR/libicu.so" 2>/dev/null
        APEX_DIRS="$RAM_DIR:$APEX_DIRS"
    fi
fi

if [ -n "$APEX_DIRS" ]; then
    export LD_LIBRARY_PATH="$APEX_DIRS:/system/lib64:/vendor/lib64:/system/lib:/vendor/lib:$LD_LIBRARY_PATH"
fi

# Clean up temporary in-memory nodes on exit
trap 'rm -f "$MEM_BIN" "$RAM_DIR/libicu.so" 2>/dev/null; if [ "$IS_TMPFS_MNT" = "1" ]; then umount -l "$RAM_DIR" 2>/dev/null; rmdir "$RAM_DIR" 2>/dev/null; fi' EXIT INT TERM HUP

# Execute directly into memory, maintaining interactive stdin/stdout
exec "$MEM_BIN" "$@"
__PAYLOAD_BEGIN__
EOF

# Strip binary before packing if strip tools exist
if command -v llvm-strip >/dev/null 2>&1; then
    llvm-strip --strip-all "$BIN_PATH" 2>/dev/null || true
elif command -v aarch64-linux-android-strip >/dev/null 2>&1; then
    aarch64-linux-android-strip --strip-all "$BIN_PATH" 2>/dev/null || true
elif command -v strip >/dev/null 2>&1; then
    strip --strip-all "$BIN_PATH" 2>/dev/null || true
fi

# Compress payload using gzip (drastically reduces .sh size from ~3.9MB to ~1.6MB)
if command -v gzip >/dev/null 2>&1; then
    echo "[*] Compressing binary payload with gzip -9..."
    gzip -9 -c "$BIN_PATH" | base64 >> "$OUT_PATH"
else
    echo "[*] Appending binary payload with base64..."
    base64 "$BIN_PATH" >> "$OUT_PATH"
fi

chmod 755 "$OUT_PATH"

echo "[+] gemini-cli.sh created successfully: $(ls -lh "$OUT_PATH")"

# Compress into official distribution package gemini-cli.zip
ZIP_PATH="libs/arm64-v8a/gemini-cli.zip"
if command -v zip >/dev/null 2>&1; then
    (cd libs/arm64-v8a && zip -9 gemini-cli.zip gemini-cli.sh)
    echo "[+] gemini-cli.zip created successfully: $(ls -lh "$ZIP_PATH")"
fi
