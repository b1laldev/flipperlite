#!/bin/bash
# Offline holatda ishga tushurish uchun: MiFi WiFi'siga ulang va:
#   bash ~/Documents/code/wi-fi/tools/recon_offline.sh
# Natija: captures/mf680/ papkasiga saqlanadi. Internet kerak emas.

DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$DIR" || exit 1

echo "=== MF680 recon (offline) ==="
python3 tools/recon.py --base http://192.168.100.1 --user admin --pass admin
echo
echo "=== Tayyor. Natijalar: $DIR/captures/mf680/ ==="
ls -la "$DIR/captures/mf680" 2>/dev/null
