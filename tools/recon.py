#!/usr/bin/env python3
""" MF680 admin panelini tahlil qilish: interfeys, HTTP, HTML/JS/CSS, yashirin SMS.

Firmwarega tegmaydi — faqat HTTP so'rovlari yuboradi va natijani captures/ ga saqlaydi.
Foydalanish:
    python3 tools/recon.py --base http://192.168.100.1 --user admin --pass admin
"""

import argparse
import json
import re
import sys
from http.cookiejar import CookieJar
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.parse import urlencode, urljoin, urlparse
from urllib.request import (
    HTTPBasicAuthHandler,
    HTTPCookieProcessor,
    HTTPRedirectHandler,
    Request,
    build_opener,
)

KEYWORDS = [
    "goform", "sms", "read_sms", "send_sms", "sms_list", "sms_send",
    "ussd", "display:none", "display: none", "visibility:hidden",
    "type=\"hidden\"", "get_sms", "delete_sms", "cmd=",
]
ASSET_RE = re.compile(r"""(?:src|href)\s*=\s*["']([^"']+\.(?:js|css)(?:\?[^"']*)?)["']""", re.I)
FORM_RE = re.compile(r"<form\b[^>]*>", re.I)
ACTION_RE = re.compile(r"""action\s*=\s*["']([^"']+)["']""", re.I)
INPUT_RE = re.compile(r"<input\b[^>]*>", re.I)
NAME_RE = re.compile(r"""name\s*=\s*["']([^"']+)["']""", re.I)


def safe_name(url):
    path = urlparse(url).path.strip("/") or "index.html"
    return re.sub(r"[^A-Za-z0-9_.-]", "_", path)


def make_opener(user, password, basic=False):
    jar = CookieJar()
    handlers = [HTTPCookieProcessor(jar), HTTPRedirectHandler()]
    if basic:
        mgr = HTTPBasicAuthHandler()
        mgr.add_password(None, "http://192.168.100.1", user, password)
        handlers.append(mgr)
    return build_opener(*handlers), jar


def fetch(opener, url, data=None, timeout=6):
    req = Request(url, data=data, headers={"User-Agent": "MF680-recon/1.0"})
    with opener.open(req, timeout=timeout) as resp:
        return resp.status, resp.read(), resp.geturl()


def try_login(opener, base, user, password):
    """Avval oddiy GET, keyin 401 bo'lsa Basic, keyin topilgan login form."""
    try:
        status, body, _ = fetch(opener, base)
        return status, body, "anonymous"
    except HTTPError as exc:
        if exc.code != 401:
            raise
    try:
        status, body, _ = fetch(opener, base)
        return status, body, "basic-auth"
    except HTTPError:
        pass

    # Forma orqali kirish: action va input nomlarini sahifadan o'qiramiz
    status, body, _ = fetch(opener, base)
    text = body.decode("utf-8", "replace")
    for form in FORM_RE.findall(text):
        action_match = ACTION_RE.search(form)
        if not action_match or "login" not in action_match.group(1).lower():
            continue
        action = urljoin(base, action_match.group(1))
        fields = {}
        for tag in INPUT_RE.findall(text):
            name_match = NAME_RE.search(tag)
            if not name_match:
                continue
            name = name_match.group(1)
            low = name.lower()
            if any(k in low for k in ("user", "name", "account")):
                fields[name] = user
            elif any(k in low for k in ("pass", "pwd", "secret")):
                fields[name] = password
            else:
                fields[name] = ""
        if not fields:
            fields = {"username": user, "password": password}
        try:
            status, body, _ = fetch(opener, action, urlencode(fields).encode())
            return status, body, "form:" + action
        except HTTPError:
            continue
    return None, b"", "login failed"


def analyze(files, out_dir):
    findings = {k: [] for k in KEYWORDS}
    endpoints = set()
    for path in files:
        text = path.read_text(encoding="utf-8", errors="replace")
        for number, line in enumerate(text.splitlines(), 1):
            low = line.lower()
            for key in KEYWORDS:
                if key.lower() in low and len(findings[key]) < 40:
                    findings[key].append({"file": path.name, "line": number, "text": line.strip()[:240]})
            for match in re.finditer(r"""["'](/[A-Za-z0-9_./?=&%:-]*(?:goform|sms|SMS)[A-Za-z0-9_./?=&%:-]*)["']""", line):
                endpoints.add(match.group(1))
    result = {
        "keywords": {k: v for k, v in findings.items() if v},
        "candidate_endpoints": sorted(endpoints),
    }
    (out_dir / "analysis.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", default="http://192.168.100.1")
    parser.add_argument("--user", default="admin")
    parser.add_argument("--pass", dest="password", default="admin")
    parser.add_argument("--basic", action="store_true", help="avval HTTP Basic sinab ko'rish")
    parser.add_argument("--out", default="captures/mf680")
    args = parser.parse_args()

    out_dir = Path(__file__).resolve().parent.parent / args.out
    out_dir.mkdir(parents=True, exist_ok=True)
    opener, jar = make_opener(args.user, args.password, args.basic)

    print("[1] Ulanish tekshiruvi")
    print("    maqsad:", args.base)
    print("    host  :", urlparse(args.base).hostname)
    try:
        status, body, method = try_login(opener, args.base, args.user, args.password)
    except URLError as exc:
        print("    XATO: qurilmaga yetib borilmadi:", exc, file=sys.stderr)
        print("    Avval MiFi WiFi'siga ulaning yoki USB tarmoq interfeysini tekshiring.", file=sys.stderr)
        return 2
    if status is None:
        print("    XATO: login qilmadi", file=sys.stderr)
        return 3
    print("    OK: HTTP %s, %d bayt, usul=%s" % (status, len(body), method))
    print("    cookie lar:", [c.name for c in jar])

    print("[2] HTML/JS/CSS yuklab olish")
    saved = []
    index = out_dir / "index.html"
    index.write_bytes(body)
    saved.append(index)
    text = body.decode("utf-8", "replace")
    assets = sorted(set(urljoin(args.base, u) for u in ASSET_RE.findall(text)))
    print("    topilgan assetlar:", len(assets))
    for url in assets:
        try:
            _, data, _ = fetch(opener, url)
        except (HTTPError, URLError) as exc:
            print("    ! %s -> %s" % (url, exc))
            continue
        path = out_dir / safe_name(url)
        path.write_bytes(data)
        saved.append(path)
        print("    +", path.name, len(data), "bayt")

    print("[3] Tahlil")
    result = analyze(saved, out_dir)
    for key, hits in result["keywords"].items():
        print("    %-18s %d ta" % (key, len(hits)))
    print("    endpoint nomzodlari:", result["candidate_endpoints"] or "topilmadi")
    print("\nNatijalar:", out_dir)
    print("Keyingi qadam: sms_client.py shu papkadagi analysis.json ni o'qiydi.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
