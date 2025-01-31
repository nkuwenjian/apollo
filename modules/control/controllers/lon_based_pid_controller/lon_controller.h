/******************************************************************************
 * Copyright 2017 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

/**
 * @file
 * @brief Defines the LonController class.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "modules/common_msgs/chassis_msgs/chassis.pb.h"
#include "modules/common_msgs/config_msgs/vehicle_config.pb.h"
#include "modules/control/controllers/lon_based_pid_controller/proto/lon_based_pid_controller_conf.pb.h"

#include "cyber/cyber.h"
#include "cyber/plugin_manager/plugin_manager.h"
#include "modules/common/filters/digital_filter.h"
#include "modules/common/filters/digital_filter_coefficients.h"
#include "modules/control/control_component/controller_task_base/common/interpolation_2d.h"
#include "modules/control/control_component/controller_task_base/common/leadlag_controller.h"
#include "modules/control/control_component/controller_task_base/common/pid_controller.h"
#include "modules/control/control_component/controller_task_base/common/trajectory_analyzer.h"
#include "modules/control/control_component/controller_task_base/control_task.h"
#include "modules/control/controllers/lon_based_pid_controller/util/check_pit.h"

/**
 * @namespace apollo::control
 * @brief apollo::control
 */
namespace apollo {
namespace control {

/**
 * @class LonController
 *
 * @brief Longitudinal controller, to compute brake / throttle values.
 */
class LonController : public ControlTask {
 public:
  /**
   * @brief constructor
   */
  LonController();

  /**
   * @brief destructor
   */
  virtual ~LonController();

  /**
   * @brief initialize Longitudinal Controller
   * @param control_conf control configurations
   * @return Status initialization status
   */
  common::Status Init(std::shared_ptr<DependencyInjector> injector) override;

  /**
   * @brief compute brake / throttle values based on current vehicle status
   *        and target trajectory
   * @param localization vehicle location
   * @param chassis vehicle status e.g., speed, acceleration
   * @param trajectory trajectory generated by planning
   * @param cmd control command
   * @return Status computation status
   */
  common::Status ComputeControlCommand(
      const localization::LocalizationEstimate *localization,
      const canbus::Chassis *chassis, const planning::ADCTrajectory *trajectory,
      control::ControlCommand *cmd) override;

  /**
   * @brief reset longitudinal controller
   * @return Status reset status
   */
  common::Status Reset() override;

  /**
   * @brief stop longitudinal controller
   */
  void Stop() override;

  /**
   * @brief longitudinal controller name
   * @return string controller name in string
   */
  std::string Name() const override;

 protected:
  void ComputeLongitudinalErrors(const TrajectoryAnalyzer *trajectory,
                                 const double preview_time, const double ts,
                                 SimpleLongitudinalDebug *debug);

  void GetPathRemain(SimpleLongitudinalDebug *debug);

  std::shared_ptr<DependencyInjector> injector_;

 private:
  void SetDigitalFilterPitchAngle();

  void InitControlCalibrationTable();

  void SetDigitalFilter(double ts, double cutoff_freq,
                        common::DigitalFilter *digital_filter);

  bool IsStopByDestination(SimpleLongitudinalDebug *debug);

  bool IsPedestrianStopLongTerm(SimpleLongitudinalDebug *debug);

  bool IsFullStopLongTerm(SimpleLongitudinalDebug *debug);

  void SetParkingBrake(const LonBasedPidControllerConf *conf,
                       control::ControlCommand *control_command);

  void CloseLogFile();

  const localization::LocalizationEstimate *localization_ = nullptr;
  const canbus::Chassis *chassis_ = nullptr;

  std::unique_ptr<Interpolation2D> control_interpolation_;
  const planning::ADCTrajectory *trajectory_message_ = nullptr;
  std::unique_ptr<TrajectoryAnalyzer> trajectory_analyzer_;

  std::string name_;
  bool controller_initialized_ = false;

  double previous_acceleration_ = 0.0;
  double reference_spd_ = 0.0;
  double reference_spd_cmd_ = 0.0;
  double previous_acceleration_reference_ = 0.0;

  PIDController speed_pid_controller_;
  PIDController station_pid_controller_;

  LeadlagController speed_leadlag_controller_;
  LeadlagController station_leadlag_controller_;

  FILE *speed_log_file_ = nullptr;

  common::DigitalFilter digital_filter_pitch_angle_;

  LonBasedPidControllerConf lon_based_pidcontroller_conf_;
  calibration_table calibration_table_;

  // vehicle parameter
  common::VehicleParam vehicle_param_;

  bool is_stop_by_pedestrian_ = false;
  bool is_stop_by_pedestrian_previous_ = false;
  double start_time_ = 0.0;
  double wait_time_diff_ = 0.0;

  bool is_full_stop_previous_ = false;
  double is_full_stop_start_time_ = 0.0;
  double is_full_stop_wait_time_diff_ = 0.0;

  bool epb_on_change_switch_ = true;
  bool epb_off_change_switch_ = true;
  int epb_change_count_ = 0;
  int smode_num_ = 0;

  double standstill_narmal_acceleration_ = 0.0;
  double stop_gain_acceleration_ = 0.0;
  double max_path_remain_when_stopped_ = 0.0;
  bool parking_release_ = false;
};

// 1.2 当前类声明为插件
CYBER_PLUGIN_MANAGER_REGISTER_PLUGIN(apollo::control::LonController,
                                     ControlTask)

}  // namespace control
}  // namespace apollo
