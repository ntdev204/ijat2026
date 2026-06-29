# Startup scripts

Thư mục này chứa scripts và systemd service để chạy `rai_web_api` theo role thiết bị.

## Scripts

| File | Role |
| --- | --- |
| `common_env.sh` | Thiết lập ROS 2 và network env chung |
| `laptop_startup.sh` | Hub/server API |
| `pi_startup.sh` | Raspberry Pi API agent |
| `jetson_startup.sh` | Jetson API agent |
| `pi_startup.service` | systemd service cho Pi |
| `jetson_startup.service` | systemd service cho Jetson |

## Chạy thủ công

```bash
bash scripts/startup/laptop_startup.sh
bash scripts/startup/pi_startup.sh
bash scripts/startup/jetson_startup.sh
```

## Cài systemd

Trên Pi:

```bash
chmod +x /home/rai/ijat2026/scripts/startup/pi_startup.sh
sudo cp /home/rai/ijat2026/scripts/startup/pi_startup.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable pi_startup.service
sudo systemctl start pi_startup.service
journalctl -u pi_startup.service -f
```

Trên Jetson:

```bash
chmod +x /home/rai/ijat2026/scripts/startup/jetson_startup.sh
sudo cp /home/rai/ijat2026/scripts/startup/jetson_startup.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable jetson_startup.service
sudo systemctl start jetson_startup.service
journalctl -u jetson_startup.service -f
```

Chi tiết xem [docs/03_van_hanh_he_thong.md](../../docs/03_van_hanh_he_thong.md).
