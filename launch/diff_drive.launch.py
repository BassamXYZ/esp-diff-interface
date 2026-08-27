from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='spirob_diff_drive',
            executable='diff_drive_teleop',
            name='diff_drive_teleop',
            output='screen'
        )
    ])
