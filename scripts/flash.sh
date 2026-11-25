#!/usr/bin/env bash
# Flash firmware to a connected ESP32-S3/C3 board.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET="${FLITE_TARGET:-esp32s3}"
PORT="${FLITE_PORT:-}"
MONITOR="${FLITE_MONITOR:-0}"

usage() {
    cat <<'EOF'
Usage: scripts/flash.sh [esp32s3|esp32c3] [--port DEVICE] [--monitor]

Options:
  -t, --target TARGET   esp32s3 (default) or esp32c3
  -p, --port DEVICE     Serial port; auto-detected when omitted
      --monitor         Open idf.py monitor after flashing

Environment: FLITE_TARGET, FLITE_PORT, FLITE_MONITOR=1, IDF_PATH
Examples:
  scripts/flash.sh esp32s3
  scripts/flash.sh esp32c3 --port /dev/cu.usbmodem1101 --monitor
EOF
}

die() {
    printf 'xato: %s\n' "$*" >&2
    exit 1
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        esp32s3|esp32c3)
            TARGET="$1"
            shift
            ;;
        -t|--target)
            [[ $# -ge 2 ]] || die "$1 uchun qiymat kerak"
            TARGET="$2"
            shift 2
            ;;
        -p|--port)
            [[ $# -ge 2 ]] || die "$1 uchun qiymat kerak"
            PORT="$2"
            shift 2
            ;;
        --monitor)
            MONITOR=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "noma'lum argument: $1"
            ;;
    esac
done

case "$TARGET" in
    esp32s3|esp32c3) ;;
    *) die "target faqat esp32s3 yoki esp32c3 bo'lishi kerak" ;;
esac

if ! command -v idf.py >/dev/null 2>&1; then
    if [[ -n "${IDF_PATH:-}" && -f "${IDF_PATH}/export.sh" ]]; then
        # shellcheck disable=SC1091
        source "${IDF_PATH}/export.sh"
    else
        die "idf.py topilmadi. Avval '. \$IDF_PATH/export.sh' ni bajaring yoki scripts/doctor.sh ni ishga tushiring."
    fi
fi

detect_port() {
    local found="" candidate first="" count=0
    if [[ "$(uname -s)" == "Darwin" ]]; then
        for candidate in /dev/cu.usbmodem* /dev/cu.usbserial-* /dev/cu.usbserial*; do
            [[ -e "$candidate" ]] || continue
            found+="${candidate}"$'\n'
        done
    else
        for candidate in /dev/ttyUSB* /dev/ttyACM* /dev/tty.usb*; do
            [[ -e "$candidate" ]] || continue
            found+="${candidate}"$'\n'
        done
    fi
    while IFS= read -r candidate; do
        [[ -n "$candidate" ]] || continue
        count=$((count + 1))
        first="$candidate"
    done <<< "$found"

    if [[ $count -eq 1 ]]; then
        printf '%s' "$first"
        return 0
    fi
    if [[ $count -gt 1 ]]; then
        printf 'xato: bir necha USB port topildi:\n%s--port yoki FLITE_PORT bilan tanlang.\n' "$found" >&2
        exit 2
    fi
    printf 'xato: USB port topilmadi. Kabelni ulang yoki --port DEVICE bering.\n' >&2
    exit 2
}

if [[ -z "$PORT" ]]; then
    PORT="$(detect_port)"
fi

export IDF_TARGET="$TARGET"
cd "$ROOT/firmware"

printf 'Target: %s\nPort:   %s\n' "$TARGET" "$PORT"
idf.py set-target "$TARGET"
idf.py build
idf.py -p "$PORT" flash

if [[ "$MONITOR" == "1" ]]; then
    exec idf.py -p "$PORT" monitor
fi
printf '\nTayyor. Monitor uchun: idf.py -p %s monitor\n' "$PORT"
