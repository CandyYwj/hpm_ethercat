import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    package_share = get_package_share_directory('hpm_gateway_control')
    parameters = os.path.join(
        package_share, 'config', 'hpm_gateway_control.yaml')

    return LaunchDescription([
        Node(
            package='hpm_gateway_control',
            executable='hpm_gateway_control_node',
            name='hpm_gateway_control_node',
            output='screen',
            parameters=[parameters],
        ),
    ])
