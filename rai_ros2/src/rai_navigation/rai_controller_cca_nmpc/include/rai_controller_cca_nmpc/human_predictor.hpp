#ifndef RAI_CONTROLLER_CCA_NMPC__HUMAN_PREDICTOR_HPP_
#define RAI_CONTROLLER_CCA_NMPC__HUMAN_PREDICTOR_HPP_

#include <vector>

#include "rai_controller_cca_nmpc/ccanmpc_types.hpp"

namespace rai_controller_cca_nmpc
{

class HumanPredictor
{
public:
  explicit HumanPredictor(const CcanmpcParameters & parameters);

  PredictedHumanContext predictHumanContext(
    const std::vector<HumanState> & humans,
    double x,
    double y,
    double robot_vx_world,
    double robot_vy_world,
    double time_from_now) const;

private:
  struct SingleHumanPrediction
  {
    double distance{0.0};
    double speed{0.0};
    double direction_cos{0.0};
    double confidence{0.0};
    double phi_h{0.0};
    bool valid{false};
  };

  const CcanmpcParameters & params_;

  double computeCovarianceConfidence(const HumanState & human) const;
  double computeEffectiveConfidence(const HumanState & human) const;
  double computePhi(const SingleHumanPrediction & prediction) const;
};

}  // namespace rai_controller_cca_nmpc

#endif  // RAI_CONTROLLER_CCA_NMPC__HUMAN_PREDICTOR_HPP_
