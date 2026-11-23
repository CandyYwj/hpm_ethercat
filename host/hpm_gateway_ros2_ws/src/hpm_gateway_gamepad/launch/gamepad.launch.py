# Launch the event-driven gamepad node with the packaged default parameters.
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    config = os.path.join(
        get_package_share_directory("hpm_gateway_gamepad"),
        "config",
        "gamepad.yaml",
    )
    return LaunchDescription([
        Node(
            package="hpm_gateway_gamepad",
            executable="gamepad_node",
            name="gamepad",
            output="screen",
            parameters=[config],
        ),
    ])
