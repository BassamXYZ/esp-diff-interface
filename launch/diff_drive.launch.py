"""
diff_drive.launch.py
────────────────────
Launches everything needed on the laptop to control the ESP32 robot:
  1. micro-ROS agent (bridges WiFi ↔ ROS2)
  2. robot_state_publisher (URDF → TF)
  3. ros2_control controller_manager
  4. joint_state_broadcaster
  5. diff_drive_controller

Usage:
  ros2 launch esp32_diff_hw_interface diff_drive.launch.py \
    robot_description_file:=/path/to/your/robot.urdf.xacro
"""

from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    ExecuteProcess,
    RegisterEventHandler,
    TimerAction,
)
from launch.event_handlers import OnProcessStart
from launch.substitutions import (
    Command,
    FindExecutable,
    LaunchConfiguration,
    PathJoinSubstitution,
)
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
import os


def generate_launch_description():

    # ── Arguments ────────────────────────────────────────────
    robot_description_file_arg = DeclareLaunchArgument(
        "robot_description_file",
        description="Absolute path to the robot URDF/Xacro file",
    )

    controllers_yaml_arg = DeclareLaunchArgument(
        "controllers_yaml",
        default_value=PathJoinSubstitution([
            FindPackageShare("esp32_diff_hw_interface"),
            "config", "controllers.yaml"
        ]),
        description="Path to the controllers YAML file",
    )

    agent_port_arg = DeclareLaunchArgument(
        "agent_port",
        default_value="8888",
        description="micro-ROS agent UDP port (match AGENT_PORT in ESP32 config.h)",
    )

    # ── Robot description from Xacro ─────────────────────────
    robot_description_content = Command([
        FindExecutable(name="xacro"), " ",
        LaunchConfiguration("robot_description_file"),
    ])
    robot_description = {"robot_description": robot_description_content}

    # ── 1. micro-ROS agent ────────────────────────────────────
    micro_ros_agent = ExecuteProcess(
        cmd=[
            "ros2", "run", "micro_ros_agent", "micro_ros_agent",
            "udp4", "--port", LaunchConfiguration("agent_port"), "-v4"
        ],
        output="screen",
        name="micro_ros_agent",
    )

    # ── 2. Robot State Publisher ──────────────────────────────
    robot_state_publisher = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        output="screen",
        parameters=[robot_description],
    )

    # ── 3. ros2_control Controller Manager ───────────────────
    controller_manager = Node(
        package="controller_manager",
        executable="ros2_control_node",
        parameters=[
            robot_description,
            LaunchConfiguration("controllers_yaml"),
        ],
        output="screen",
    )

    # ── 4 & 5. Spawn controllers after controller_manager ────
    # Wait 2 s so the controller_manager is ready
    spawn_jsb = TimerAction(
        period=2.0,
        actions=[
            Node(
                package="controller_manager",
                executable="spawner",
                arguments=["joint_state_broadcaster", "--controller-manager", "/controller_manager"],
                output="screen",
            )
        ],
    )

    spawn_diff_drive = TimerAction(
        period=3.0,
        actions=[
            Node(
                package="controller_manager",
                executable="spawner",
                arguments=["diff_drive_controller", "--controller-manager", "/controller_manager"],
                output="screen",
            )
        ],
    )

    return LaunchDescription([
        robot_description_file_arg,
        controllers_yaml_arg,
        agent_port_arg,
        micro_ros_agent,
        robot_state_publisher,
        controller_manager,
        spawn_jsb,
        spawn_diff_drive,
    ])
