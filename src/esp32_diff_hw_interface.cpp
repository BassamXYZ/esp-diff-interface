// =============================================================
//  esp32_diff_hw_interface.cpp
// =============================================================

#include "esp32_diff_hw_interface/esp32_diff_hw_interface.hpp"

#include <hardware_interface/types/hardware_interface_type_values.hpp>
#include <pluginlib/class_list_macros.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <std_msgs/msg/float64_multi_array.hpp>

#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace esp32_diff_hw_interface
{

// =============================================================
//  on_init  —  read parameters from ros2_control URDF tag
// =============================================================
hardware_interface::CallbackReturn
Esp32DiffHwInterface::on_init(const hardware_interface::HardwareInfo & info)
{
  if (hardware_interface::SystemInterface::on_init(info) !=
      hardware_interface::CallbackReturn::SUCCESS)
  {
    return hardware_interface::CallbackReturn::ERROR;
  }

  // Expect exactly 2 joints
  if (info_.joints.size() != 2) {
    RCLCPP_FATAL(rclcpp::get_logger("Esp32DiffHwInterface"),
      "Expected 2 joints, got %zu", info_.joints.size());
    return hardware_interface::CallbackReturn::ERROR;
  }

  for (size_t i = 0; i < 2; ++i) {
    joint_names_[i] = info_.joints[i].name;

    // Each joint must expose exactly one velocity command interface
    if (info_.joints[i].command_interfaces.size() != 1 ||
        info_.joints[i].command_interfaces[0].name !=
          hardware_interface::HW_IF_VELOCITY)
    {
      RCLCPP_FATAL(rclcpp::get_logger("Esp32DiffHwInterface"),
        "Joint '%s' must have exactly one velocity command interface",
        joint_names_[i].c_str());
      return hardware_interface::CallbackReturn::ERROR;
    }

    // Each joint must expose position and velocity state interfaces
    bool has_pos = false, has_vel = false;
    for (const auto & si : info_.joints[i].state_interfaces) {
      if (si.name == hardware_interface::HW_IF_POSITION) has_pos = true;
      if (si.name == hardware_interface::HW_IF_VELOCITY)  has_vel = true;
    }
    if (!has_pos || !has_vel) {
      RCLCPP_FATAL(rclcpp::get_logger("Esp32DiffHwInterface"),
        "Joint '%s' must have position and velocity state interfaces",
        joint_names_[i].c_str());
      return hardware_interface::CallbackReturn::ERROR;
    }
  }

  RCLCPP_INFO(rclcpp::get_logger("Esp32DiffHwInterface"),
    "Joints: [%s, %s]", joint_names_[0].c_str(), joint_names_[1].c_str());

  return hardware_interface::CallbackReturn::SUCCESS;
}

// =============================================================
//  on_configure  —  create ROS2 node + pub/sub
// =============================================================
hardware_interface::CallbackReturn
Esp32DiffHwInterface::on_configure(const rclcpp_lifecycle::State & /*prev*/)
{
  node_ = rclcpp::Node::make_shared("esp32_diff_hw_interface_node");

  // ── Publisher: /joint_vel_cmd → ESP32 ──────────────────────
  vel_cmd_pub_ = node_->create_publisher<std_msgs::msg::Float64MultiArray>(
    "joint_vel_cmd", rclcpp::QoS(10).reliable());

  // ── Subscriber: /joint_states ← ESP32 ─────────────────────
  auto sub = node_->create_subscription<sensor_msgs::msg::JointState>(
    "joint_states",
    rclcpp::QoS(10).best_effort(),   // micro-ROS publishes BEST_EFFORT
    [this](const sensor_msgs::msg::JointState::SharedPtr msg) {
      jointStateCallback(msg);
    }
  );
  // Store type-erased to avoid template in header
  joint_state_sub_erased_ = sub;

  RCLCPP_INFO(node_->get_logger(), "Configured — waiting for ESP32 joint_states…");
  return hardware_interface::CallbackReturn::SUCCESS;
}

// =============================================================
//  on_activate / on_deactivate
// =============================================================
hardware_interface::CallbackReturn
Esp32DiffHwInterface::on_activate(const rclcpp_lifecycle::State & /*prev*/)
{
  hw_positions_.fill(0.0);
  hw_velocities_.fill(0.0);
  hw_commands_.fill(0.0);
  RCLCPP_INFO(node_->get_logger(), "Activated");
  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn
Esp32DiffHwInterface::on_deactivate(const rclcpp_lifecycle::State & /*prev*/)
{
  // Send zero velocity to stop robot
  std_msgs::msg::Float64MultiArray stop_msg;
  stop_msg.data = {0.0, 0.0};
  vel_cmd_pub_->publish(stop_msg);

  RCLCPP_INFO(node_->get_logger(), "Deactivated — motors stopped");
  return hardware_interface::CallbackReturn::SUCCESS;
}

// =============================================================
//  Interface registration
// =============================================================
std::vector<hardware_interface::StateInterface>
Esp32DiffHwInterface::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> ifaces;
  for (size_t i = 0; i < 2; ++i) {
    ifaces.emplace_back(joint_names_[i],
      hardware_interface::HW_IF_POSITION, &hw_positions_[i]);
    ifaces.emplace_back(joint_names_[i],
      hardware_interface::HW_IF_VELOCITY, &hw_velocities_[i]);
  }
  return ifaces;
}

std::vector<hardware_interface::CommandInterface>
Esp32DiffHwInterface::export_command_interfaces()
{
  std::vector<hardware_interface::CommandInterface> ifaces;
  for (size_t i = 0; i < 2; ++i) {
    ifaces.emplace_back(joint_names_[i],
      hardware_interface::HW_IF_VELOCITY, &hw_commands_[i]);
  }
  return ifaces;
}

// =============================================================
//  read  —  copy latest ESP32 state into hw_* buffers
// =============================================================
hardware_interface::return_type
Esp32DiffHwInterface::read(const rclcpp::Time & /*t*/,
                           const rclcpp::Duration & /*dt*/)
{
  // Spin the node once to process any pending /joint_states messages
  rclcpp::spin_some(node_);
  // hw_positions_ and hw_velocities_ updated by jointStateCallback
  return hardware_interface::return_type::OK;
}

// =============================================================
//  write  —  send commands to ESP32
// =============================================================
hardware_interface::return_type
Esp32DiffHwInterface::write(const rclcpp::Time & /*t*/,
                            const rclcpp::Duration & /*dt*/)
{
  std_msgs::msg::Float64MultiArray cmd_msg;
  cmd_msg.data.resize(2);
  cmd_msg.data[0] = hw_commands_[0];  // left  rad/s
  cmd_msg.data[1] = hw_commands_[1];  // right rad/s
  vel_cmd_pub_->publish(cmd_msg);

  return hardware_interface::return_type::OK;
}

// =============================================================
//  /joint_states subscriber callback
// =============================================================
void Esp32DiffHwInterface::jointStateCallback(
  const sensor_msgs::msg::JointState::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(state_mutex_);

  for (size_t m = 0; m < msg->name.size(); ++m) {
    for (size_t j = 0; j < 2; ++j) {
      if (msg->name[m] == joint_names_[j]) {
        if (m < msg->position.size())
          hw_positions_[j] = msg->position[m];
        if (m < msg->velocity.size())
          hw_velocities_[j] = msg->velocity[m];
      }
    }
  }
}

}  // namespace esp32_diff_hw_interface

// ── pluginlib export ──────────────────────────────────────────
PLUGINLIB_EXPORT_CLASS(
  esp32_diff_hw_interface::Esp32DiffHwInterface,
  hardware_interface::SystemInterface
)
