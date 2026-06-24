#include "rai_human_perception/yolo_detector.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

#ifdef RAI_HUMAN_PERCEPTION_HAS_TENSORRT
#include <NvInfer.h>
#endif

namespace rai_human_perception
{
namespace
{

constexpr int kPersonClassId = 0;

std::string quote(const std::string & value)
{
  return "'" + value + "'";
}

}  

YoloDetector::YoloDetector(YoloDetectorOptions options)
: options_(std::move(options))
{
}

bool YoloDetector::initialize()
{
  if (initialized_) {
    return true;
  }

  if (!ensureEngine()) {
    return false;
  }

  initialized_ = loadTensorRtEngine();
  if (!initialized_) {
    initialized_ = loadOpenCvFallback();
  }
  return initialized_;
}

bool YoloDetector::ensureEngine()
{
  if (options_.engine_path.empty() || std::filesystem::exists(options_.engine_path)) {
    return true;
  }
  if (!options_.auto_export_engine || options_.weights_path.empty() || options_.export_script.empty()) {
    return true;
  }

  std::ostringstream command;
  command << "python3 " << quote(options_.export_script)
          << " --weights " << quote(options_.weights_path)
          << " --imgsz " << options_.image_size
          << " --output " << quote(options_.engine_path);
  if (options_.fp16) {
    command << " --fp16";
  }
  return std::system(command.str().c_str()) == 0;
}

bool YoloDetector::loadTensorRtEngine()
{
  if (options_.engine_path.empty() || !std::filesystem::exists(options_.engine_path)) {
    return false;
  }

#ifdef RAI_HUMAN_PERCEPTION_HAS_TENSORRT
  std::ifstream engine_file(options_.engine_path, std::ios::binary);
  if (!engine_file.good()) {
    return false;
  }
  
  
  
  return false;
#else
  return false;
#endif
}

bool YoloDetector::loadOpenCvFallback()
{
  const std::string model_path =
    !options_.onnx_path.empty() && std::filesystem::exists(options_.onnx_path) ?
    options_.onnx_path :
    options_.weights_path;
  if (model_path.empty() || !std::filesystem::exists(model_path)) {
    return false;
  }

  net_ = cv::dnn::readNet(model_path);
  if (net_.empty()) {
    return false;
  }
  net_.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
  net_.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
  using_tensorrt_ = false;
  return true;
}

std::vector<Detection2D> YoloDetector::detect(const cv::Mat & bgr_image)
{
  if (!initialized_ || bgr_image.empty()) {
    return {};
  }

  cv::Mat blob;
  cv::dnn::blobFromImage(
    bgr_image,
    blob,
    1.0 / 255.0,
    cv::Size(options_.image_size, options_.image_size),
    cv::Scalar(),
    true,
    false);
  net_.setInput(blob);

  std::vector<cv::Mat> outputs;
  net_.forward(outputs, net_.getUnconnectedOutLayersNames());
  return parseOpenCvDetections(outputs, bgr_image.size());
}

std::vector<Detection2D> YoloDetector::parseOpenCvDetections(
  const std::vector<cv::Mat> & outputs,
  const cv::Size & image_size) const
{
  std::vector<cv::Rect> boxes;
  std::vector<float> confidences;
  std::vector<int> class_ids;

  for (const auto & output : outputs) {
    cv::Mat rows = output;
    if (rows.dims == 3) {
      rows = rows.reshape(1, rows.size[1]);
    }
    if (rows.cols < 6) {
      continue;
    }

    for (int row = 0; row < rows.rows; ++row) {
      const float * data = rows.ptr<float>(row);
      float objectness = data[4];
      int class_id = 0;
      float class_score = 1.0F;
      if (rows.cols > 6) {
        cv::Mat scores(1, rows.cols - 5, CV_32FC1, const_cast<float *>(data + 5));
        cv::Point class_location;
        double max_score = 0.0;
        cv::minMaxLoc(scores, nullptr, &max_score, nullptr, &class_location);
        class_id = class_location.x;
        class_score = static_cast<float>(max_score);
      }
      const float confidence = objectness * class_score;
      if (class_id != kPersonClassId || confidence < options_.confidence_threshold) {
        continue;
      }

      const int cx = static_cast<int>(data[0] * image_size.width);
      const int cy = static_cast<int>(data[1] * image_size.height);
      const int width = static_cast<int>(data[2] * image_size.width);
      const int height = static_cast<int>(data[3] * image_size.height);
      boxes.emplace_back(cx - width / 2, cy - height / 2, width, height);
      confidences.push_back(confidence);
      class_ids.push_back(class_id);
    }
  }

  std::vector<int> keep;
  cv::dnn::NMSBoxes(boxes, confidences, options_.confidence_threshold, options_.nms_threshold, keep);

  std::vector<Detection2D> detections;
  detections.reserve(keep.size());
  int id = 0;
  for (int idx : keep) {
    cv::Rect box = boxes[static_cast<size_t>(idx)] & cv::Rect(0, 0, image_size.width, image_size.height);
    if (box.area() <= 0) {
      continue;
    }
    detections.push_back({class_ids[static_cast<size_t>(idx)], id++, confidences[static_cast<size_t>(idx)], box});
  }
  return detections;
}

}  
