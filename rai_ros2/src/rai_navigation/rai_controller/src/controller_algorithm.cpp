#include "rai_controller/controller_algorithm.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <unordered_map>

namespace rai_controller
{
namespace
{

std::unordered_map<std::string, ControllerFactory> & registry()
{
  static std::unordered_map<std::string, ControllerFactory> controllers;
  return controllers;
}

std::vector<std::string> & controllerIds()
{
  static std::vector<std::string> ids;
  return ids;
}

}  // namespace

std::string normalizeControllerId(std::string value)
{
  for (auto & ch : value) {
    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
  }
  if (value == "CCANMPC" || value == "CCA-NMPC") {
    return "CCA_NMPC";
  }
  return value.empty() ? "CCA_NMPC" : value;
}

const std::vector<std::string> & availableControllerIds()
{
  return controllerIds();
}

std::unique_ptr<ControllerAlgorithm> createController(
  const std::string & id,
  const ControllerConfig & config)
{
  const std::string normalized = normalizeControllerId(id);
  const auto it = registry().find(normalized);
  if (it == registry().end()) {
    return nullptr;
  }
  return it->second(config);
}

void registerController(const std::string & id, ControllerFactory factory)
{
  const std::string normalized = normalizeControllerId(id);
  if (!factory) {
    throw std::invalid_argument("controller factory is null");
  }
  auto & controllers = registry();
  if (controllers.find(normalized) == controllers.end()) {
    controllerIds().push_back(normalized);
    std::sort(controllerIds().begin(), controllerIds().end());
  }
  controllers[normalized] = factory;
}

}  // namespace rai_controller
