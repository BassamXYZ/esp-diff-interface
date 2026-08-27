#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

class DiffDriveTeleop : public rclcpp::Node
{
public:
  DiffDriveTeleop() : Node("diff_drive_teleop")
  {
    pub_ = this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);
    RCLCPP_INFO(this->get_logger(), "Diff Drive Teleop started.");
    RCLCPP_INFO(this->get_logger(), "Controls: w/s (forward/back), a/d (turn), space (stop), q (quit)");
    // Set up non-blocking keyboard input
    tcgetattr(STDIN_FILENO, &orig_termios_);
    struct termios new_termios = orig_termios_;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
  }

  ~DiffDriveTeleop()
  {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios_);
  }

  void run()
  {
    rclcpp::Rate rate(10);
    while (rclcpp::ok())
    {
      char c = getchar();
      if (c != EOF)
      {
        handle_key(c);
      }
      rclcpp::spin_some(this->get_node_base_interface());
      rate.sleep();
    }
  }

private:
  void handle_key(char key)
  {
    auto msg = geometry_msgs::msg::Twist();
    switch (key)
    {
      case 'w': msg.linear.x = 0.5;  break;
      case 's': msg.linear.x = -0.5; break;
      case 'a': msg.angular.z = 1.0; break;
      case 'd': msg.angular.z = -1.0; break;
      case ' ': msg.linear.x = 0.0; msg.angular.z = 0.0; break;
      case 'q': rclcpp::shutdown(); return;
      default: return;
    }
    pub_->publish(msg);
  }

  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr pub_;
  struct termios orig_termios_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<DiffDriveTeleop>();
  node->run();
  rclcpp::shutdown();
  return 0;
}
