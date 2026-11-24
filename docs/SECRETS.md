# Sirlar va xavfsiz konfiguratsiya

Repo **public** bo'lgani uchun hech qanday parol, token yoki shaxsiy ma'lumot committed bo'lmasligi
kerak.

## Qoidalar

1. Haqiqiy sozlamalar faqat `.env` faylida yoki muhit o'zgaruvchilarida bo'ladi.
   `.env` va `.env.*` gitignore qilingan; faqat `.env.example` (placeholder'lar) repo ichida.
2. `.env` dagi qiymatlar real environment o'zgaruvchilari bilan **ustun qo'yilmaydi** —
   environment har doim ustun turadi (CI/server uchun qulay).
3. `FLITE_API_TOKEN` ixtiyoriy. Qo'yilsa, barcha `/api/v1/*` endpointlari
   `Authorization: Bearer <token>` talab qiladi. `/health` va veb-assets ochiq qoladi.
4. Token kamida 16 belgi bo'lishi shart. Yaratish:

   ```bash
   openssl rand -hex 32
   ```

5. Loglarga faqat qurilma **host** yoziladi; token, WiFi paroli va `.env` hech qachon
   loglanmaydi (`config.Config.SafeHost()`).
6. `FLITE_CORS_ORIGIN` default `*` (mahalliy tarmoq uchun). Internetga ochayotganda
   aniq origin qo'ying, masalan `http://192.168.1.20:8080`.

## Mijoz tomonida token

- **Veb-panel:** token input maydoniga kiritiladi va faqat brauzer `localStorage` da saqlanadi.
  Bu oddiy saqlash usuli — umumiy kompyuterda ishlatayotganda tokenni o'chiring.
- **Android:** token ilova ichidagi yashirin maydonga kiritiladi va Activity holatida tutuladi;
  har so'rovda `Authorization` header yuboriladi.
- **Boshqa ilovalar:** [EXTERNAL_API.md](EXTERNAL_API.md) dagi misollarga qarang.

## Qurilma tomoni

- `POST /api/v1/wifi` orqali yuborilgan WiFi ma'lumotlari **faqat o'zingizning** tarmog'ingizga
  tegishli bo'lishi kerak. Hozirda ular faqat ESP32 RAM/IDF configida ishlatiladi, doimiy
  saqlanmaydi. Keyingi qadamda NVS ga shifrlab yozish qo'shiladi.
- SoftAP ochiq (parolsiz) — bu faqat mahalliy sozlash tarmog'i. Uni Internetga ko'priklamang.
-agar qurilma yoki bridge Internetga ochilsa, token + HTTPS/TLS reverse proxy majburiy.

## Agar token tasodifan repo ga tushib qolsa

Darhol yangisini yarating, eskisini `FLITE_API_TOKEN` dan olib tashlang, git history'ni
tozalang (`git filter-repo` yoki BFG) va eski tokenni bekor qiling.
