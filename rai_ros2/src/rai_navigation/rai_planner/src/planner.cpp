#include "rai_planner/planner.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <unordered_map>

namespace rai_planner
{
namespace
{

std::unordered_map<std::string, PlannerFactory> & registry()
{
  static std::unordered_map<std::string, PlannerFactory> planners;
  return planners;
}

std::vector<std::string> & plannerIds()
{
  static std::vector<std::string> ids;
  return ids;
}

}  

std::string normalizePlannerId(std::string value)
{
  for (auto & ch : value) {
    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
  }
  if (value == "ASTAR") {
    return "A_STAR";
  }
  return value.empty() ? "A_STAR" : value;
}

const std::vector<std::string> & availablePlannerIds()
{
  return plannerIds();
}

std::unique_ptr<Planner> createPlanner(const std::string & id)
{
  const std::string normalized = normalizePlannerId(id);
  const auto it = registry().find(normalized);
  if (it == registry().end()) {
    return nullptr;
  }
  return it->second();
}

void registerPlanner(const std::string & id, PlannerFactory factory)
{
  const std::string normalized = normalizePlannerId(id);
  if (!factory) {
    throw std::invalid_argument("planner factory is null");
  }
  auto & planners = registry();
  if (planners.find(normalized) == planners.end()) {
    plannerIds().push_back(normalized);
    std::sort(plannerIds().begin(), plannerIds().end());
  }
  planners[normalized] = factory;
}

}  
