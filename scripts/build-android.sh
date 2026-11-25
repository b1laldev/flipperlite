#!/usr/bin/env bash
# Build (and optionally install) the Android debug APK.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODE="debug"
INSTALL=0
CLEAN=0

usage() {
    cat <<'EOF'
Usage: scripts/build-android.sh [--release] [--clean] [--install]

Options:
      --release   Build app-release.apk instead of app-debug.apk
      --clean     Run gradle clean before the build
      --install   Install the built APK on the connected device via adb
  -h, --help      Show this help

Environment: ANDROID_HOME / ANDROID_SDK_ROOT (or android/local.properties)
EOF
}

die() {
    printf 'xato: %s\n' "$*" >&2
    exit 1
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --release) MODE="release"; shift ;;
        --clean) CLEAN=1; shift ;;
        --install) INSTALL=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *) die "noma'lum argument: $1" ;;
    esac
done

cd "$ROOT/android"

command -v java >/dev/null 2>&1 || die "JDK 17 kerak (java topilmadi)"
[[ -x ./gradlew ]] || die "./gradlew topilmadi yoki ijro etilmaydi"
printf 'Java: %s\n' "$(java -version 2>&1 | head -n 1)"

sdk="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"
if [[ -z "$sdk" && ! -f local.properties ]]; then
    printf "diqqat: ANDROID_HOME/ANDROID_SDK_ROOT va local.properties topilmadi; Android Studio sozlamasini tekshiring\n" >&2
fi

if [[ "$INSTALL" == "1" ]]; then
    command -v adb >/dev/null 2>&1 || die "--install uchun adb kerak"
    adb get-state >/dev/null 2>&1 || die "hech qanday Android qurilma ulanmagan"
fi

tasks=()
[[ "$CLEAN" == "1" ]] && tasks+=("clean")
if [[ "$MODE" == "release" ]]; then
    tasks+=(":app:assembleRelease")
    apk="app/build/outputs/apk/release/app-release.apk"
else
    tasks+=(":app:assembleDebug")
    apk="app/build/outputs/apk/debug/app-debug.apk"
fi

./gradlew "${tasks[@]}"
[[ -f "$apk" ]] || die "APK yaratilmadi: $apk"
printf '\nAPK tayyor: %s\n' "$ROOT/android/$apk"

if [[ "$INSTALL" == "1" ]]; then
    adb install -r "$apk"
    printf "Ilova qurilmaga o'rnatildi.\n"
fi
