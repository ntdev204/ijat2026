echo  'KERNEL=="ttyUSB*", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60",ATTRS{serial}=="0002", MODE:="0777", GROUP:="dialout", SYMLINK+="rai_controller"' >/etc/udev/rules.d/rai_controller.rules
echo  'KERNEL=="ttyCH343USB*", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="55d4",ATTRS{serial}=="0002", MODE:="0777", GROUP:="dialout", SYMLINK+="rai_controller"' >/etc/udev/rules.d/rai_controller2.rules
echo  'KERNEL=="ttyACM*", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="55d4",ATTRS{serial}=="0002", MODE:="0777", GROUP:="dialout", SYMLINK+="rai_controller"' >/etc/udev/rules.d/rai_controller3.rules

echo  'KERNEL=="ttyUSB*", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60",ATTRS{serial}=="0001", MODE:="0777", GROUP:="dialout", SYMLINK+="rai_lidar"' >/etc/udev/rules.d/rai_lidar.rules
echo  'KERNEL=="ttyCH343USB*", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="55d4",ATTRS{serial}=="0001", MODE:="0777", GROUP:="dialout", SYMLINK+="rai_lidar"' >/etc/udev/rules.d/rai_lidar2.rules
echo  'KERNEL=="ttyACM*", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="55d4",ATTRS{serial}=="0001", MODE:="0777", GROUP:="dialout", SYMLINK+="rai_lidar"' >/etc/udev/rules.d/rai_lidar3.rules
echo  'KERNEL=="ttyACM*", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="55d4",ATTRS{serial}=="5AA6052218", MODE:="0777", GROUP:="dialout", SYMLINK+="rai_lidar"' >/etc/udev/rules.d/rai_lidar_wch.rules

echo  'KERNEL=="ttyUSB*", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60",ATTRS{serial}=="0003", MODE:="0777", GROUP:="dialout", SYMLINK+="rai_FDI_IMU_GNSS"' >/etc/udev/rules.d/rai_fdi_imu_gnss.rules
echo  'KERNEL=="ttyCH343USB*", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="55d4",ATTRS{serial}=="0003", MODE:="0777", GROUP:="dialout", SYMLINK+="rai_FDI_IMU_GNSS"' >/etc/udev/rules.d/rai_fdi_imu_gnss2.rules
echo  'KERNEL=="ttyACM*", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="55d4",ATTRS{serial}=="0003", MODE:="0777", GROUP:="dialout", SYMLINK+="rai_FDI_IMU_GNSS"' >/etc/udev/rules.d/rai_fdi_imu_gnss3.rules
echo  'KERNEL=="ttyUSB*", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60",ATTRS{serial}=="0003", MODE:="0777", GROUP:="dialout", SYMLINK+="rai_IMU"' >/etc/udev/rules.d/rai_imu.rules

echo 'KERNEL=="ttyCH343USB*", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="55d4", ATTRS{serial}=="0003", MODE:="0777", GROUP:="dialout", SYMLINK+="rai_IMU"' >/etc/udev/rules.d/rai_imu_343.rules

echo  'KERNEL=="ttyACM*", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="55d4", ATTRS{serial}=="0003", MODE:="0777", GROUP:="dialout", SYMLINK+="rai_IMU"' >/etc/udev/rules.d/rai_imu_ACM.rules

echo  'KERNEL=="ttyACM*", ATTRS{idVendor}=="1546", ATTRS{idProduct}=="01a9", MODE:="0777", GROUP:="dialout", SYMLINK+="rai_gnss"' >/etc/udev/rules.d/rai_gnss.rules
echo  'KERNEL=="ttyUSB*", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60",ATTRS{serial}=="0005", MODE:="0777", GROUP:="dialout", SYMLINK+="rai_gnss"' >/etc/udev/rules.d/rai_gps.rules

echo  'KERNEL=="ttyCH343USB*", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="55d4",ATTRS{serial}=="0004", MODE:="0777", GROUP:="dialout", SYMLINK+="rai_mic"' >>/etc/udev/rules.d/rai_mic.rules
echo  'KERNEL=="ttyACM*", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="55d4",ATTRS{serial}=="0004", MODE:="0777", GROUP:="dialout", SYMLINK+="rai_mic"' >>/etc/udev/rules.d/rai_mic.rules

echo  'SUBSYSTEM=="video4linux",ATTR{name}=="GENERAL WEBCAM",ATTR{index}=="0",MODE:="0777",SYMLINK+="RgbCam"' >>/etc/udev/rules.d/camera.rules
echo  'SUBSYSTEM=="video4linux",ATTR{name}=="GENERAL WEBCAM: GENERAL WEBCAM",ATTR{index}=="0",MODE:="0777",SYMLINK+="RgbCam"' >>/etc/udev/rules.d/camera.rules
echo  'SUBSYSTEM=="video4linux",ATTR{name}=="Astra Pro HD Camera: Astra Pro ",ATTR{index}=="0",MODE:="0777",SYMLINK+="Astra_Pro"' >>/etc/udev/rules.d/camera.rules
echo  'SUBSYSTEM=="video4linux",ATTRS{idVendor}=="2bc5" ATTRS{idProduct}=="050e",ATTR{index}=="0",MODE:="0777",SYMLINK+="Astra_Dabai"' >>/etc/udev/rules.d/camera.rules
echo  'SUBSYSTEM=="video4linux",ATTRS{idVendor}=="2bc5" ATTRS{idProduct}=="0511",ATTR{index}=="0",MODE:="0777",SYMLINK+="Astra_Gemini"' >>/etc/udev/rules.d/camera.rules
echo  'SUBSYSTEM=="video4linux",ATTR{name}=="Intel(R) RealSense(TM) Depth Ca",ATTR{index}=="0",MODE:="0777",SYMLINK+="realsense"' >>/etc/udev/rules.d/camera.rules
echo  'SUBSYSTEM=="video4linux",ATTR{name}=="Integrated Webcam: Integrated W",ATTR{index}=="0",MODE:="0777",SYMLINK+="RgbCam"' >>/etc/udev/rules.d/camera.rules

service udev reload
sleep 2
service udev restart


