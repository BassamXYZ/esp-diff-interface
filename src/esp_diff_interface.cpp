#include <memory>
#include <string>
#include <vector>

#include "hardware_interface/system_interface.hpp"
#include "hardware_interface/handle.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "hardware_interface/types/lifecycle_state_names.hpp"  // for CallbackReturn
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "geometry_msgs/msg/twist.hpp"

using hardware_interface::return_type;

namespace esp_diff
{

class ESPDiffDrive : public hardware_interface::SystemInterface
{
public:
  ESPDiffDrive() = default;

  // Override on_init instead of init
  CallbackReturn on_init(const hardware_interface::HardwareInfo & info) override
  {
    // Call the base class implementation to store info_
    CallbackReturn ret = hardware_interface::SystemInterface::on_init(info);
    if (ret != CallbackReturn::SUCCESS)
      return ret;

    // Retrieve wheel joint names from URDF parameters
    left_joint_ = info_.hardware_parameters["left_wheel_name"];
    right_joint_ = info_.hardware_parameters["right_wheel_name"];

    // Create ROS node for communication with ESP32
    node_ = std::make_shared<rclcpp::Node>("esp_interface");

    // Subscriber to ESP32's joint states (topic: /joint_states)
    joint_state_sub_ = node_->create_subscription<sensor_msgs::msg::JointState>(
      "/joint_states", 10,
      std::bind(&ESPDiffDrive::joint_state_callback, this, std::placeholders::_1)
    );

    // Publisher to command the ESP32 (topic: /cmd_vel)
    cmd_vel_pub_ = node_->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 10);

    // Initialize state variables
    left_pos_ = 0.0; left_vel_ = 0.0;
    right_pos_ = 0.0; right_vel_ = 0.0;

    return CallbackReturn::SUCCESS;
  }

  // Export state interfaces (position, velocity for each wheel)
  std::vector<hardware_interface::StateInterface> export_state_interfaces() override
  {
    std::vector<hardware_interface::StateInterface> state_interfaces;
    state_interfaces.emplace_back(left_joint_, hardware_interface::HW_IF_POSITION, &left_pos_);
    state_interfaces.emplace_back(left_joint_, hardware_interface::HW_IF_VELOCITY, &left_vel_);
    state_interfaces.emplace_back(right_joint_, hardware_interface::HW_IF_POSITION, &right_pos_);
    state_interfaces.emplace_back(right_joint_, hardware_interface::HW_IF_VELOCITY, &right_vel_);
    return state_interfaces;
  }

  // Export command interfaces (velocity commands)
  std::vector<hardware_interface::CommandInterface> export_command_interfaces() override
  {
    std::vector<hardware_interface::CommandInterface> command_interfaces;
    command_interfaces.emplace_back(left_joint_, hardware_interface::HW_IF_VELOCITY, &left_cmd_);
    command_interfaces.emplace_back(right_joint_, hardware_interface::HW_IF_VELOCITY, &right_cmd_);
    return command_interfaces;
  }

  // Read: update state from the latest joint_state message
  return_type read(const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/) override
  {
    // Process any incoming joint_state messages
    rclcpp::spin_some(node_);
    return return_type::OK;
  }

  // Write: publish command velocities to ESP32 as a Twist message
  return_type write(const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/) override
  {
    geometry_msgs::msg::Twist twist;
    // Convert left/right wheel rad/s to linear/angular using robot kinematics
    double wheel_radius = 0.032;   // [m]
    double wheel_sep = 0.150;      // [m] track width
    twist.linear.x = (right_cmd_ + left_cmd_) * wheel_radius / 2.0;
    twist.angular.z = (right_cmd_ - left_cmd_) * wheel_radius / wheel_sep;
    cmd_vel_pub_->publish(twist);
    return return_type::OK;
  }

private:
  std::string left_joint_, right_joint_;
  double left_pos_, left_vel_, right_pos_, right_vel_;
  double left_cmd_ = 0.0, right_cmd_ = 0.0;

  rclcpp::Node::SharedPtr node_;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_state_sub_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;

  void joint_state_callback(const sensor_msgs::msg::JointState::SharedPtr msg)
  {
    for (size_t i = 0; i < msg->name.size(); ++i) {
      if (msg->name[i] == left_joint_) {
        left_pos_ = msg->position[i];
        left_vel_ = msg->velocity[i];
      } else if (msg->name[i] == right_joint_) {
        right_pos_ = msg->position[i];
        right_vel_ = msg->velocity[i];
      }
    }
  }
};

}  // namespace esp_diff

#include "pluginlib/class_list_macros.hpp"
PLUGINLIB_EXPORT_CLASS(esp_diff::ESPDiffDrive, hardware_interface::SystemInterface)
