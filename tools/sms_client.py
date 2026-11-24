#!/usr/bin/env python3
"""MF680 SMS veb-API mijozi — recon.py topgan endpointlar asosida ishlaydi.

Hech qanday endpointni taxmin qilmaydi: topilgan nomzodlarni ko'rsatadi,
aniq URL ni --url bilan siz berasiz. Firmware/tizim fayllariga tegmaydi.

Misollar:
    python3 tools/sms_client.py endpoints
    python3 tools/sms_client.py unhide
    python3 tools/sms_client.py list --url '/goform/goform_get_cmd_process?cmd=GET_SMS_LIST'
    python3 tools/sms_client.py read 3 --url '/goform/goform_get_cmd_process?cmd=READ_SMS&id={id}'
    python3 tools/sms_client.py send +998901234567 "Salom" \
        --url '/goform/goform_set_cmd_process' \
        --data 'cmd=SEND_SMS&to={to}&text={text}'
"""

import argparse
import json
import sys
from http.cookiejar import CookieJar
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.parse import urljoin, urlencode
from urllib.request import (
    HTTPBasicAuthHandler,
    HTTPCookieProcessor,
    HTTPRedirectHandler,
    Request,
    build_opener,
)

BASE_DIR = Path(__file__).resolve().parent.parent
DEFAULT_CAPTURE = BASE_DIR / "captures" / "mf680"

UNHIDE_CSS = """
<style id="mf680-unhide">
  [style*="display:none"], [style*="display: none"],
  [style*="visibility:hidden"], [style*="visibility: hidden"] {
    display: block !important;
    visibility: visible !important;
  }
  .hidden, .hide, [hidden] { display: block !important; visibility: visible !important; }
</style>
"""


def load_analysis(capture):
    path = capture / "analysis.json"
    if not path.exists():
        print("Avval: python3 tools/recon.py ni ishga tushuring (%s yo'q)" % path, file=sys.stderr)
        return {}
    return json.loads(path.read_text(encoding="utf-8"))


def make_opener(base, user, password, basic=False, login_form=None):
    jar = CookieJar()
    handlers = [HTTPCookieProcessor(jar), HTTPRedirectHandler()]
    if basic:
        mgr = HTTPBasicAuthHandler()
        mgr.add_password(None, base, user, password)
        handlers.append(mgr)
    return build_opener(*handlers), jar


def request(opener, base, path, method="GET", data=None, timeout=8):
    url = urljoin(base.rstrip("/") + "/", path.lstrip("/"))
    payload = data.encode("utf-8") if isinstance(data, str) else data
    req = Request(url, data=payload, method=method, headers={"User-Agent": "MF680-sms/1.0"})
    with opener.open(req, timeout=timeout) as resp:
        return resp.status, resp.read()


def cmd_endpoints(capture):
    result = load_analysis(capture)
    eps = result.get("candidate_endpoints", [])
    sms = [e for e in eps if "sms" in e.lower()]
    return sms, eps


def cmd_unhide(capture, base):
    index = capture / "index.html"
    if not index.exists():
        print("index.html topilmadi:", index, file=sys.stderr)
        return 2
    html = index.read_text(encoding="utf-8", errors="replace")
    injection = '<base href="%s/">\n%s' % (base.rstrip("/"), UNHIDE_CSS)
    if "<head" in html.lower():
        pos = html.lower().find(">") + 1 if "<head" in html.lower() else 0
        html = html[:pos] + injection + html[pos:]
    else:
        html = injection + html
    out = capture / "index_unhidden.html"
    out.write_text(html, encoding="utf-8")
    print("Yaratildi:", out)
    print("Eslatma: server tomonidan yashirilgan bo'limlar CSS bilan chiqmasligi mumkin.")
    return 0


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("command", choices=["endpoints", "unhide", "list", "read", "send", "raw"])
    p.add_argument("args", nargs="*", help="read: <id>; send: <raqam> <matn>")
    p.add_argument("--base", default="http://192.168.100.1")
    p.add_argument("--user", default="admin")
    p.add_argument("--pass", dest="password", default="admin")
    p.add_argument("--basic", action="store_true")
    p.add_argument("--capture", default=str(DEFAULT_CAPTURE))
    p.add_argument("--url", help="recon topgan aniq endpoint (masalan /goform/goform_get_cmd_process?cmd=...)")
    p.add_argument("--method", default="GET", choices=["GET", "POST"])
    p.add_argument("--data", help="POST uchun shablon: cmd=SEND_SMS&to={to}&text={text}")
    args = p.parse_args()
    capture = Path(args.capture)

    if args.command == "endpoints":
        sms, all_eps = cmd_endpoints(capture)
        print("SMS bilan bog'liq nomzodlar (%d):" % len(sms))
        for e in sms:
            print("  ", e)
        print("\nBoshqa goform/endpointlar (%d):" % len(all_eps))
        for e in all_eps[:40]:
            print("  ", e)
        return 0

    if args.command == "unhide":
        return cmd_unhide(capture, args.base)

    if not args.url:
        sms, _ = cmd_endpoints(capture)
        print("--url kerak. Topilgan SMS nomzodlari:", file=sys.stderr)
        for e in sms:
            print("  ", e, file=sys.stderr)
        return 2

    opener, jar = make_opener(args.base, args.user, args.password, args.basic)
    path = args.url
    data = None

    if args.command == "read" and args.args:
        path = path.replace("{id}", args.args[0])
    if args.command == "send":
        if len(args.args) < 2:
            print("Foydalanish: send <raqam> <matn>", file=sys.stderr)
            return 2
        to, text = args.args[0], " ".join(args.args[1:])
        path = path.replace("{to}", to).replace("{text}", text)
        if args.data:
            data = args.data.replace("{to}", to).replace("{text}", text)

    try:
        status, body = request(opener, args.base, path, args.method, data)
    except (HTTPError, URLError) as exc:
        print("XATO:", exc, file=sys.stderr)
        return 4
    print("HTTP", status, "|", len(body), "bayt")
    print(body.decode("utf-8", "replace")[:8000])
    print("\nCookie lar:", [c.name for c in jar])
    return 0


if __name__ == "__main__":
    sys.exit(main())
