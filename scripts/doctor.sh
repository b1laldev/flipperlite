#!/usr/bin/env bash
# Report which local tools are ready for firmware, bridge and Android builds.
set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

check_tool() {
    local name="$1" command_name="$2" hint="$3"
    if command -v "$command_name" >/dev/null 2>&1; then
        printf 'OK      %-14s %s\n' "$name" "$(command -v "$command_name")"
    else
        printf 'MISSING %-14s %s\n' "$name" "$hint"
    fi
}

check_file() {
    local name="$1" path="$2" hint="$3"
    if [[ -n "$path" && -f "$path" ]]; then
        printf 'OK      %-14s %s\n' "$name" "$path"
    else
        printf 'MISSING %-14s %s\n' "$name" "$hint"
    fi
}

printf 'FlipperLite doctor — %s\n\n' "$ROOT"
check_tool git git "git o'rnating"
check_tool go go "Go 1.22+: https://go.dev/dl/"
check_tool curl curl "curl o'rnating"
check_tool python3 python3 "python3 o'rnating (JSON/provisioning)"
check_tool idf.py idf.py "ESP-IDF: . \$IDF_PATH/export.sh"
check_file 'idf export' "${IDF_PATH:-}/export.sh" "IDF_PATH ni o'rnating"
check_tool java java "JDK 17 o'rnating"
check_tool adb adb "Android SDK platform-tools"
check_tool sdkmanager sdkmanager "Android SDK cmdline-tools"
check_file 'gradlew' "$ROOT/android/gradlew" "android/gradlew yo'q"

sdk="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"
check_file 'Android SDK' "${sdk}/platform-tools" "ANDROID_HOME yoki ANDROID_SDK_ROOT ni o'rnating"

printf '\nFirmware: scripts/flash.sh | Provisioning: scripts/provision.sh | APK: scripts/build-android.sh\n'
