# RAI Robot Operations

Repo này chứa ROS 2 workspace, web API và dashboard để vận hành robot RAI, chạy CCA-NMPC navigation và thu thập dataset thực nghiệm.

## Tài liệu chính

Bộ docs vận hành mới nằm trong [`docs/`](./docs/README.md):

- [Kiến trúc hệ thống](./docs/01_kien_truc_he_thong.md)
- [Mô hình toán và điều khiển](./docs/02_mo_hinh_toan_va_dieu_khien.md)
- [Vận hành hệ thống](./docs/03_van_hanh_he_thong.md)
- [Chạy dataset theo kịch bản](./docs/04_chay_dataset_theo_kich_ban.md)
- [Web dashboard và API](./docs/05_web_dashboard_va_api.md)
- [Troubleshooting](./docs/06_troubleshooting.md)

## Chạy nhanh

```bash
# Hub database
make db-up

# Hub API
bash scripts/startup/laptop_startup.sh

# Website
cd rai_website
npm install
npm run dev
```

Trên Pi và Jetson chạy script tương ứng:

```bash
bash scripts/startup/pi_startup.sh
bash scripts/startup/jetson_startup.sh
```

Mặc định website mở tại:

```text
http://100.116.199.115:3000
```
