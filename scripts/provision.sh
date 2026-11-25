#!/usr/bin/env bash
# Wait for the device API, optionally send WiFi credentials and launch an app.
# Secrets are passed through the environment, never through the command line.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Load the same .env file as the bridge, but let real environment variables win.
ENV_FILE="${FLITE_ENV_FILE:-$ROOT/server/.env}"
if [[ -n "$ENV_FILE" && -f "$ENV_FILE" ]]; then
    set -a
    # shellcheck disable=SC1090
    source "$ENV_FILE"
    set +a
fi

DEVICE_URL="${FLITE_DEVICE_URL:-http://192.168.4.1}"
BRIDGE_URL="${FLITE_BRIDGE_URL:-}"
TOKEN="${FLITE_API_TOKEN:-}"
SSID="${FLITE_WIFI_SSID:-}"
PASSWORD="${FLITE_WIFI_PASSWORD:-}"
APP_ID="${FLITE_LAUNCH_APP:-menu}"
TIMEOUT="${FLITE_WAIT_SECONDS:-120}"
AFTER_URL="${FLITE_AFTER_URL:-}"
AUTH_HEADER=""

usage() {
    cat <<'EOF'
Usage: scripts/provision.sh [options]

Options:
      --device URL          Device base URL (default FLITE_DEVICE_URL)
      --bridge URL          Go bridge base URL; uses /api/v1/* endpoints
      --token TOKEN         Bearer token when --bridge is used (FLITE_API_TOKEN)
      --wifi-ssid SSID      Own WiFi network SSID (FLITE_WIFI_SSID)
      --wifi-password PASS  Own WiFi password (FLITE_WIFI_PASSWORD)
      --launch ID           App to launch after setup (default: menu)
      --no-launch           Do not launch an app
      --after-url URL       Device URL to wait for after WiFi setup
      --timeout SECONDS     Wait timeout (default: 120)
  -h, --help                Show this help

Reads server/.env by default (override with FLITE_ENV_FILE); real env wins.
The password is converted to JSON via the environment and is never echoed.
EOF
}

die() {
    printf 'xato: %s\n' "$*" >&2
    exit 1
}

need_value() {
    [[ $# -ge 2 ]] || die "$1 uchun qiymat kerak"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --device) need_value "$@"; DEVICE_URL="$2"; shift 2 ;;
        --bridge) need_value "$@"; BRIDGE_URL="$2"; shift 2 ;;
        --token) need_value "$@"; TOKEN="$2"; shift 2 ;;
        --wifi-ssid) need_value "$@"; SSID="$2"; shift 2 ;;
        --wifi-password) need_value "$@"; PASSWORD="$2"; shift 2 ;;
        --launch) need_value "$@"; APP_ID="$2"; shift 2 ;;
        --no-launch) APP_ID=""; shift ;;
        --after-url) need_value "$@"; AFTER_URL="$2"; shift 2 ;;
        --timeout) need_value "$@"; TIMEOUT="$2"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) die "noma'lum argument: $1" ;;
    esac
done

command -v curl >/dev/null 2>&1 || die "curl kerak"
command -v python3 >/dev/null 2>&1 || die "python3 kerak (JSON yaratish uchun)"
[[ "$TIMEOUT" =~ ^[0-9]+$ ]] || die "--timeout raqam bo'lishi kerak"

strip_slash() {
    local value="$1"
    printf '%s' "${value%/}"
}

if [[ -n "$TOKEN" ]]; then
    if [[ -n "$BRIDGE_URL" ]]; then
        AUTH_HEADER="Authorization: Bearer ${TOKEN}"
    else
        printf "diqqat: token faqat --bridge bilan ishlatiladi, e'tiborsiz qoldirildi\n" >&2
    fi
fi

if [[ -n "$BRIDGE_URL" ]]; then
    BASE_URL="$(strip_slash "$BRIDGE_URL")"
    STATUS_URL="${BASE_URL}/api/v1/status"
    WIFI_URL="${BASE_URL}/api/v1/wifi"
    LAUNCH_URL="${BASE_URL}/api/v1/launch"
else
    BASE_URL="$(strip_slash "$DEVICE_URL")"
    STATUS_URL="${BASE_URL}/api/status"
    WIFI_URL="${BASE_URL}/api/wifi"
    LAUNCH_URL="${BASE_URL}/api/launch"
fi

curl_status() {
    if [[ -n "$AUTH_HEADER" ]]; then
        curl -fsS --max-time 3 -H "$AUTH_HEADER" "$1" >/dev/null 2>&1
    else
        curl -fsS --max-time 3 "$1" >/dev/null 2>&1
    fi
}

curl_post() {
    if [[ -n "$AUTH_HEADER" ]]; then
        curl -fsS --max-time 15 -H 'Content-Type: application/json' -H "$AUTH_HEADER" -d "$2" "$1" >/dev/null
    else
        curl -fsS --max-time 15 -H 'Content-Type: application/json' -d "$2" "$1" >/dev/null
    fi
}

wait_for() {
    local url="$1" label="$2" deadline=$((SECONDS + TIMEOUT))
    printf '%s kutilmoqda' "$label"
    until curl_status "$url"; do
        if [[ $SECONDS -ge $deadline ]]; then
            printf '\n'
            die "$label ${TIMEOUT}s ichida javob bermadi"
        fi
        printf '.'
        sleep 2
    done
    printf ' OK\n'
}

wait_for "$STATUS_URL" "Qurilma API"

if [[ -n "$SSID" ]]; then
    wifi_payload="$(WIFI_SSID="$SSID" WIFI_PASSWORD="$PASSWORD" python3 -c 'import json, os; print(json.dumps({"ssid": os.environ["WIFI_SSID"], "password": os.environ["WIFI_PASSWORD"]}))')"
    curl_post "$WIFI_URL" "$wifi_payload"
    printf "WiFi so'rov yuborildi (parol ko'rsatilmadi)\n"

    if [[ -n "$AFTER_URL" ]]; then
        wait_for "$(strip_slash "$AFTER_URL")/api/status" "Yangi manzil"
    fi
fi

if [[ -n "$APP_ID" ]]; then
    launch_payload="$(FLITE_APP_ID="$APP_ID" python3 -c 'import json, os; print(json.dumps({"id": os.environ["FLITE_APP_ID"]}))')"
    curl_post "$LAUNCH_URL" "$launch_payload"
    printf "App ishga tushirildi: %s\n" "$APP_ID"
fi

printf 'Provisioning tugadi.\n'
