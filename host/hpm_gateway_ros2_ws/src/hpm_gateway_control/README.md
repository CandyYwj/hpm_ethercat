# HPM Gateway Control

This package separates ROS 2 communication from the EtherCAT cyclic master.

The ROS 2 executor thread owns subscriptions and publishers. The EtherCAT
worker thread owns SOEM, the raw EtherCAT socket, PDO exchange, and the 1 kHz
absolute-time cycle. The two threads exchange fixed-size snapshots through
bounded buffers.

The default ROS feedback publish rate is 200 Hz. The EtherCAT PDO cycle remains
1 kHz, so each ROS feedback message publishes the latest sampled PDO snapshot.

## Build on RDK

```bash
cd ~/hpm_gateway_ros2_ws
source /opt/tros/humble/setup.bash
colcon build --symlink-install
source install/setup.bash
```

The RDK default SOEM root is `/root/Tools/EtherCat/SOEM`. The package searches
that path, `/usr/local`, and common SOEM build output directories, so the normal
build command above should be enough once the SOEM library exists.

If the build still reports that SOEM was not found, first locate the library:

```bash
find /root/Tools/EtherCat/SOEM /usr/local \( -name 'libsoem.so*' -o -name 'libsoem.a' \) -print
```

## Safe ROS-only mode

This is the default. It does not open EtherCAT and does not send PDOs.

```bash
ros2 launch hpm_gateway_control hpm_gateway_control.launch.py
```

Topics:

```text
/hpm_gateway/motor_command    hpm_gateway_msgs/msg/MotorMitCommand
/hpm_gateway/motor_commands   hpm_gateway_msgs/msg/MotorMitCommandArray
/hpm_gateway/motor_feedback   hpm_gateway_msgs/msg/MotorFeedbackArray
/imu/data                     sensor_msgs/msg/Imu
/imu/temperature              sensor_msgs/msg/Temperature
```

The installed reference command sender publishes the same four-axis velocity
commands for CAN motor IDs 1, 2, 3, and 13 as
`host/rdk_ecat_motor_rt/ecat_motor_rt.c` at 200 Hz by default. PDO axis n is
CAN motor ID n + 1; the gateway firmware routes that ID to its configured CAN
bus and protocol:

```bash
ros2 run hpm_gateway_control send_reference_motor_commands.py
```

The firmware PDO exposes YESENSE quaternion values as `q0..q3`, where `q0` is
the scalar component. The ROS publisher maps that to `orientation.w,x,y,z` and
normalizes the quaternion before publishing.

## EtherCAT mode

Starting EtherCAT requires `arm_all_axes: true` in the config file because the
current gateway firmware may enable configured motors after its first command
PDO. The current RDK interface is configured as `eth0` in
`config/hpm_gateway_control_eth0.yaml`; keep startup arguments out of the command
line and change that YAML file when the interface or timing needs to change.

```bash
sudo -E bash -lc '
source /opt/tros/humble/setup.bash
source ~/hpm_gateway_ros2_ws/install/setup.bash
ros2 launch hpm_gateway_control hpm_gateway_control_eth0.launch.py
'
```

The command timeout clears the PDO command snapshot after the configured
interval. This is a communication fallback, not an independent physical
emergency stop.
