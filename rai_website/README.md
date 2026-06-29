# RAI Website

Next.js dashboard cho hệ thống RAI Robot. Dashboard dùng để giám sát runtime, điều khiển robot, quản lý navigation và chạy dataset.

## Cấu hình

Tạo `.env.local`:

```bash
NEXT_PUBLIC_API_URL=http://100.116.199.115:8080
NEXT_PUBLIC_ALLOWED_DEV_ORIGINS=100.116.199.115
```

Frontend chỉ gọi hub API. Hub `rai_web_api` tự relay REST/WebSocket xuống `rai_runtime_bridge` trên Pi/Jetson theo `RAI_PI_BRIDGE_URL` và `RAI_JETSON_BRIDGE_URL`.

## Chạy development

```bash
npm install
npm run dev
```

Mở:

```text
http://100.116.199.115:3000
```

## Chạy production

```bash
npm run build
npm run start -- --hostname 0.0.0.0 --port 3000
```

## Tài liệu liên quan

- [Web dashboard và API](../docs/05_web_dashboard_va_api.md)
- [Vận hành hệ thống](../docs/03_van_hanh_he_thong.md)
- [Chạy dataset theo kịch bản](../docs/04_chay_dataset_theo_kich_ban.md)
