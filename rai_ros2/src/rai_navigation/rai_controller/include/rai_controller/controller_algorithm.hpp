#ifndef RAI_CONTROLLER__CONTROLLER_ALGORITHM_HPP_
#define RAI_CONTROLLER__CONTROLLER_ALGORITHM_HPP_

#include <memory>
#include <string>
#include <vector>

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "nav_msgs/msg/path.hpp"
#include "rai_controller_cca_nmpc/ccanmpc_types.hpp"

namespace rai_controller
{

using ControllerHumanState = rai_controller_cca_nmpc::HumanState;
using ControllerConfig = rai_controller_cca_nmpc::CcanmpcParameters;
using ControllerContext = rai_controller_cca_nmpc::ContextState;
using ControllerResult = rai_controller_cca_nmpc::SolveResult;

class ControllerAlgorithm
{
public:
  virtual ~ControllerAlgorithm() = default;
  virtual std::string id() const = 0;
  virtual ControllerResult solve(
    const geometry_msgs::msg::PoseStamped & robot_pose,
    const geometry_msgs::msg::Twist & robot_velocity,
    const nav_msgs::msg::Path & reference_path,
    const std::vector<ControllerHumanState> & humans,
    const ControllerContext & context,
    const nav_msgs::msg::OccupancyGrid * costmap) = 0;
  virtual void reset() = 0;
};

using ControllerFactory = std::unique_ptr<ControllerAlgorithm> (*)(const ControllerConfig & config);

std::string normalizeControllerId(std::string value);
const std::vector<std::string> & availableControllerIds();
std::unique_ptr<ControllerAlgorithm> createController(
  const std::string & id,
  const ControllerConfig & config);
void registerController(const std::string & id, ControllerFactory factory);

}  // namespace rai_controller

#endif  // RAI_CONTROLLER__CONTROLLER_ALGORITHM_HPP_
