#pragma once
// =============================================================
//  esp32_diff_hw_interface.hpp
//
//  ros2_control hardware interface plugin that bridges:
//    /joint_vel_cmd  ──►  ESP32 (via micro-ROS)
//    /joint_states   ◄──  ESP32 (via micro-ROS)
//
//  Pluginlib type: "esp32_diff_hw_interface/Esp32DiffHwInterface"
// =============================================================

#include <hardware_interface/system_interface.hpp>
#include <hardware_interface/handle.hpp>
#include <hardware_interface/hardware_info.hpp>
#include <hardware_interface/types/hardware_interface_type_values.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/state.hpp>

#include <sensor_msgs/msg/joint_state.hpp>
#include <std_msgs/msg/float64_multi_array.hpp>

#include <array>
#include <mutex>
#include <string>

namespace esp32_diff_hw_interface
{

class Esp32DiffHwInterface : public hardware_interface::SystemInterface
{
public:
  RCLCPP_SHARED_PTR_DEFINITIONS(Esp32DiffHwInterface)

  // ── Lifecycle ─────────────────────────────────────────────
  hardware_interface::CallbackReturn on_init(
    const hardware_interface::HardwareInfo & info) override;

  hardware_interface::CallbackReturn on_configure(
    const rclcpp_lifecycle::State & previous_state) override;

  hardware_interface::CallbackReturn on_activate(
    const rclcpp_lifecycle::State & previous_state) override;

  hardware_interface::CallbackReturn on_deactivate(
    const rclcpp_lifecycle::State & previous_state) override;

  // ── Interface registration ─────────────────────────────────
  std::vector<hardware_interface::StateInterface>   export_state_interfaces()   override;
  std::vector<hardware_interface::CommandInterface> export_command_interfaces() override;

  // ── Read / Write ──────────────────────────────────────────
  hardware_interface::return_type read(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

  hardware_interface::return_type write(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

private:
  // Joint names come from the ros2_control URDF tag
  std::array<std::string, 2> joint_names_;

  // State mirrors (filled by /joint_states subscriber)
  std::array<double, 2> hw_positions_  = {0.0, 0.0};  // rad
  std::array<double, 2> hw_velocities_ = {0.0, 0.0};  // rad/s

  // Command mirrors (written by diff_drive_controller)
  std::array<double, 2> hw_commands_   = {0.0, 0.0};  // rad/s

  // ROS2 communication
  rclcpp::Node::SharedPtr node_;
  rclcpp::Publisher<std_msgs__msg__float64_multi_array>::SharedPtr vel_cmd_pub_;

  // Use a shared pointer to avoid including the full type in header
  std::shared_ptr<void> joint_state_sub_erased_;

  std::mutex state_mutex_;

  // Joint-state subscriber callback
  void jointStateCallback(
    const sensor_msgs::msg::JointState::SharedPtr msg);

  // Mapping: joint name → index in hw_positions_ / hw_velocities_
  std::array<int, 2> joint_index_ = {0, 1};
};

}  // namespace esp32_diff_hw_interface
