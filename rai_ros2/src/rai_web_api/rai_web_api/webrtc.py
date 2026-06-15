import asyncio
import fractions
import time
from typing import Optional

import av
import numpy as np
from aiortc import MediaStreamTrack
from sensor_msgs.msg import Image


class RosImageVideoTrack(MediaStreamTrack):
    """WebRTC video track lấy frame mới nhất từ WebBridgeNode."""

    kind = "video"

    def __init__(self, bridge_node, fps: int = 15):
        super().__init__()
        self.bridge_node = bridge_node
        self.frame_interval = 1.0 / max(1, fps)
        self.last_frame_time = 0.0
        self.start_time = time.time()
        self.frame_count = 0

    async def recv(self) -> av.VideoFrame:
        await self._wait_next_frame_slot()
        image_msg = await asyncio.to_thread(self._wait_for_ros_image)
        frame = self._image_msg_to_video_frame(image_msg)
        frame.pts = self.frame_count
        frame.time_base = fractions.Fraction(1, int(1 / self.frame_interval))
        self.frame_count += 1
        return frame

    async def _wait_next_frame_slot(self) -> None:
        now = time.time()
        elapsed = now - self.last_frame_time
        if elapsed < self.frame_interval:
            await asyncio.sleep(self.frame_interval - elapsed)
        self.last_frame_time = time.time()

    def _wait_for_ros_image(self) -> Image:
        self.bridge_node.camera_frame_event.wait(timeout=1.0)
        self.bridge_node.camera_frame_event.clear()
        image_msg = self.bridge_node.latest_camera_frame
        if image_msg is None:
            return self._blank_image()
        return image_msg

    def _blank_image(self) -> Image:
        image_msg = Image()
        image_msg.height = 480
        image_msg.width = 640
        image_msg.encoding = "rgb8"
        image_msg.step = image_msg.width * 3
        image_msg.data = bytes(image_msg.height * image_msg.step)
        return image_msg

    def _image_msg_to_video_frame(self, image_msg: Image) -> av.VideoFrame:
        array = self._image_msg_to_rgb_array(image_msg)
        return av.VideoFrame.from_ndarray(array, format="rgb24")

    def _image_msg_to_rgb_array(self, image_msg: Image) -> np.ndarray:
        height = image_msg.height
        width = image_msg.width
        encoding = image_msg.encoding.lower()
        raw = np.frombuffer(image_msg.data, dtype=np.uint8)

        if encoding in {"rgb8", "bgr8"}:
            image = raw.reshape((height, image_msg.step // 3, 3))[:, :width, :]
            if encoding == "bgr8":
                return image[:, :, ::-1].copy()
            return image.copy()

        if encoding in {"rgba8", "bgra8"}:
            image = raw.reshape((height, image_msg.step // 4, 4))[:, :width, :3]
            if encoding == "bgra8":
                return image[:, :, ::-1].copy()
            return image.copy()

        if encoding in {"mono8", "8uc1"}:
            image = raw.reshape((height, image_msg.step))[:, :width]
            return np.repeat(image[:, :, None], 3, axis=2)

        return np.zeros((height, width, 3), dtype=np.uint8)
