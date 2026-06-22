#ifndef RAI_HUMAN_PERCEPTION__YOLO_DETECTOR_HPP_
#define RAI_HUMAN_PERCEPTION__YOLO_DETECTOR_HPP_

#include <string>
#include <vector>

#include <opencv2/dnn.hpp>
#include <opencv2/core.hpp>

#include "rai_human_perception/types.hpp"

namespace rai_human_perception
{

struct YoloDetectorOptions
{
  std::string engine_path;
  std::string onnx_path;
  std::string weights_path;
  std::string export_script;
  int image_size{640};
  double confidence_threshold{0.35};
  double nms_threshold{0.45};
  bool fp16{true};
  bool auto_export_engine{true};
};

class YoloDetector
{
public:
  explicit YoloDetector(YoloDetectorOptions options);

  bool initialize();
  std::vector<Detection2D> detect(const cv::Mat & bgr_image);
  bool usingTensorRt() const {return using_tensorrt_;}

private:
  bool ensureEngine();
  bool loadTensorRtEngine();
  bool loadOpenCvFallback();
  std::vector<Detection2D> parseOpenCvDetections(
    const std::vector<cv::Mat> & outputs,
    const cv::Size & image_size) const;

  YoloDetectorOptions options_;
  cv::dnn::Net net_;
  bool using_tensorrt_{false};
  bool initialized_{false};
};

}  // namespace rai_human_perception

#endif  // RAI_HUMAN_PERCEPTION__YOLO_DETECTOR_HPP_
