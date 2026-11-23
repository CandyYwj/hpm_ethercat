"""PowerLink 电源板控制节点 launch。"""
import os

from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    package_dir = get_package_share_directory("hpm_gateway_power")
    config_path = os.path.join(package_dir, "config", "power_node.yaml")

    return LaunchDescription([
        Node(
            package="hpm_gateway_power",
            executable="power_node",
            name="hpm_gateway_power_node",
            output="screen",
            parameters=[config_path],
        )
    ])
