# RAI Website

Next.js dashboard cho giám sát robot, điều khiển runtime, SLAM, navigation, RViz web, và dataset collection.

## Môi trường mặc định

`rai_website` được cấu hình để chạy trên server/hub `100.116.199.115`:

```bash
NEXT_PUBLIC_API_URL=http://100.116.199.115:8080
NEXT_PUBLIC_PI_API_URL=http://100.120.77.81:8080
NEXT_PUBLIC_JETSON_API_URL=http://100.69.39.18:8080
```

Các giá trị này hiện được lưu trong `.env.local`.

## Chạy development

```bash
cd rai_website
npm install
npm run dev
```

Frontend dev server bind vào `100.116.199.115:3000`, nên mở:

```text
http://100.116.199.115:3000
```

## Chạy production

```bash
cd rai_website
npm install
npm run build
npm run start -- --hostname 0.0.0.0 --port 3000
```

## Phụ thuộc backend

Website cần `rai_web_api` đang chạy trên server/hub ở cổng `8080`, và hub API này cần biết:

```bash
RAI_PI_API_URL=http://100.120.77.81:8080
RAI_JETSON_API_URL=http://100.69.39.18:8080
```

Hướng dẫn vận hành đầy đủ xem tại:

- `docs/device_launch_guide.md`
- `docs/2_he_thong_va_dinh_tuyen.md`
