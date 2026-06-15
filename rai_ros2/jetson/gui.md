# Tắt GUI (về console mode, giải phóng RAM)

sudo systemctl set-default multi-user.target
sudo systemctl isolate multi-user.target

# Bật lại GUI

sudo systemctl set-default graphical.target
sudo systemctl isolate graphical.target
