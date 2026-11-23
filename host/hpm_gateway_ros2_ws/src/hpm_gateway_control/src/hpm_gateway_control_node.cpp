#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <time.h>
#include <utility>

#include "builtin_interfaces/msg/time.hpp"
#include "hpm_gateway_control/gateway_ecat_api.hpp"
#include "hpm_gateway_msgs/msg/motor_feedback_array.hpp"
#include "hpm_gateway_msgs/msg/motor_mit_command.hpp"
#include "hpm_gateway_msgs/msg/motor_mit_command_array.hpp"
#include "rclcpp/executors/multi_threaded_executor.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include "sensor_msgs/msg/temperature.hpp"

namespace {

constexpr std::uint32_t kAxisCount = GATEWAY_RT_AXIS_COUNT;
constexpr std::uint32_t kImuFieldQuaternion = UINT32_C(1) << 6;
constexpr double kDegreesToRadians = 3.14159265358979323846 / 180.0;

std::uint64_t monotonic_time_us()
{
    struct timespec now {};
    (void)clock_gettime(CLOCK_MONOTONIC, &now);
    return static_cast<std::uint64_t>(now.tv_sec) * 1000000U +
           static_cast<std::uint64_t>(now.tv_nsec) / 1000U;
}

bool is_finite(float value)
{
    return std::isfinite(static_cast<double>(value));
}

bool quaternion_is_valid(const float quaternion[4])
{
    double norm_squared = 0.0;
    for (std::uint32_t index = 0U; index < 4U; ++index) {
        if (!is_finite(quaternion[index])) {
            return false;
        }
        norm_squared += static_cast<double>(quaternion[index]) *
                        static_cast<double>(quaternion[index]);
    }
    return norm_squared > 0.25 && norm_squared < 4.0;
}

}  // namespace

class HpmGatewayControlNode final : public rclcpp::Node {
public:
    HpmGatewayControlNode()
        : Node("hpm_gateway_control_node")
    {
        command_topic_ = declare_parameter<std::string>(
            "command_topic", "/hpm_gateway/motor_command");
        aggregate_command_topic_ = declare_parameter<std::string>(
            "aggregate_command_topic", "/hpm_gateway/motor_commands");
        feedback_topic_ = declare_parameter<std::string>(
            "feedback_topic", "/hpm_gateway/motor_feedback");
        imu_topic_ = declare_parameter<std::string>(
            "imu_topic", "/imu/data");
        temperature_topic_ = declare_parameter<std::string>(
            "temperature_topic", "/imu/temperature");
        imu_frame_id_ = declare_parameter<std::string>(
            "imu_frame_id", "imu_link");
        interface_name_ = declare_parameter<std::string>(
            "interface_name", "");
        start_ethercat_ = declare_parameter<bool>(
            "start_ethercat", false);
        arm_all_axes_ = declare_parameter<bool>(
            "arm_all_axes", false);
        cycle_us_ = declare_parameter<int>("cycle_us", 1000);
        rx_timeout_us_ = declare_parameter<int>("rx_timeout_us", 500);
        command_timeout_ms_ = declare_parameter<int>(
            "command_timeout_ms", 100);
        rt_priority_ = declare_parameter<int>("rt_priority", 0);
        rt_cpu_ = declare_parameter<int>("rt_cpu", -1);
        feedback_publish_hz_ = declare_parameter<double>(
            "feedback_publish_hz", 200.0);

        if (cycle_us_ < 500 || cycle_us_ > 100000 ||
            rx_timeout_us_ <= 0 || rx_timeout_us_ >= cycle_us_ ||
            command_timeout_ms_ <= 0 || rt_priority_ < 0 ||
            rt_priority_ > 99 || rt_cpu_ < -1) {
            throw std::runtime_error("invalid EtherCAT timing parameters");
        }
        if (feedback_publish_hz_ <= 0.0 ||
            feedback_publish_hz_ > 1000.0) {
            throw std::runtime_error("feedback_publish_hz must be in (0, 1000]");
        }
        if (start_ethercat_ && !arm_all_axes_) {
            throw std::runtime_error(
                "start_ethercat requires arm_all_axes=true");
        }

        if (start_ethercat_) {
            hpm_gateway_control::GatewayEcatOptions options;
            options.interface_name = interface_name_;
            options.cycle_us = static_cast<std::uint32_t>(cycle_us_);
            options.rx_timeout_us = static_cast<std::uint32_t>(rx_timeout_us_);
            options.command_timeout_ms =
                static_cast<std::uint32_t>(command_timeout_ms_);
            options.rt_priority = rt_priority_;
            options.rt_cpu = rt_cpu_;
            options.arm_all_axes = arm_all_axes_;
            ecat_ = std::make_unique<hpm_gateway_control::GatewayEcat>(
                std::move(options));
            if (!ecat_->start()) {
                throw std::runtime_error("EtherCAT master failed to start");
            }
        }

        const auto command_qos = rclcpp::QoS(rclcpp::KeepLast(1))
                                     .best_effort()
                                     .durability_volatile();
        const auto feedback_qos = rclcpp::SensorDataQoS();

        /*
         * P1: separate mutually-exclusive callback groups so the 200 Hz
         * feedback publisher never blocks the command subscribers (and vice
         * versa) under MultiThreadedExecutor. Both command subscriptions share
         * one group because they both write command_frame_; keeping them
         * serialized avoids a data race on that member.
         */
        command_callback_group_ = this->create_callback_group(
            rclcpp::CallbackGroupType::MutuallyExclusive);
        feedback_callback_group_ = this->create_callback_group(
            rclcpp::CallbackGroupType::MutuallyExclusive);

        /* P3: fixed monotonic->realtime offset, captured once at startup. */
        const auto realtime_now =
            std::chrono::system_clock::now().time_since_epoch();
        const auto monotonic_now =
            std::chrono::steady_clock::now().time_since_epoch();
        monotonic_to_realtime_offset_ns_ =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                realtime_now - monotonic_now)
                .count();

        feedback_publisher_ =
            create_publisher<hpm_gateway_msgs::msg::MotorFeedbackArray>(
                feedback_topic_, feedback_qos);
        imu_publisher_ =
            create_publisher<sensor_msgs::msg::Imu>(imu_topic_, feedback_qos);
        temperature_publisher_ =
            create_publisher<sensor_msgs::msg::Temperature>(
                temperature_topic_, feedback_qos);

        rclcpp::SubscriptionOptions command_subscription_options;
        command_subscription_options.callback_group = command_callback_group_;

        command_subscription_ =
            create_subscription<hpm_gateway_msgs::msg::MotorMitCommand>(
                command_topic_, command_qos,
                [this](const hpm_gateway_msgs::msg::MotorMitCommand::SharedPtr msg) {
                    handle_single_command(*msg);
                },
                command_subscription_options);
        aggregate_command_subscription_ =
            create_subscription<hpm_gateway_msgs::msg::MotorMitCommandArray>(
                aggregate_command_topic_, command_qos,
                [this](
                    const hpm_gateway_msgs::msg::MotorMitCommandArray::SharedPtr msg) {
                    handle_aggregate_command(*msg);
                },
                command_subscription_options);

        const auto period = std::chrono::duration<double>(
            1.0 / feedback_publish_hz_);
        feedback_timer_ = create_wall_timer(
            std::chrono::duration_cast<std::chrono::nanoseconds>(period),
            [this]() {
                publish_feedback();
            },
            feedback_callback_group_);
        status_timer_ = create_wall_timer(
            std::chrono::seconds(1),
            [this]() {
                publish_status();
            });

        RCLCPP_WARN(
            get_logger(),
            "ROS thread ready; EtherCAT thread=%s, command=%s, aggregate=%s",
            start_ethercat_ ? "started" : "disabled",
            command_topic_.c_str(),
            aggregate_command_topic_.c_str());
    }

private:
    void handle_single_command(
        const hpm_gateway_msgs::msg::MotorMitCommand & msg)
    {
        if (msg.motor_id == 0U || msg.motor_id > kAxisCount ||
            !is_finite(msg.position_rad) ||
            !is_finite(msg.velocity_rad_s) ||
            !is_finite(msg.kp) ||
            !is_finite(msg.kd) ||
            !is_finite(msg.torque_nm)) {
            RCLCPP_ERROR(get_logger(), "Rejected invalid single motor command");
            return;
        }

        const auto axis = msg.motor_id - 1U;

        /* 单轴命令 = 只驱动该轴：先清空整帧（其余 15 轴清零），再只填目标轴，
         * 避免上一帧的非零命令残留在 command_frame_ 中继续驱动其他轴。 */
        std::memset(&command_frame_.output, 0,
                    sizeof(command_frame_.output));
        auto & command = command_frame_.output.axis[axis];
        command.position_rad = msg.position_rad;
        command.velocity_rad_s = msg.velocity_rad_s;
        command.kp_nm_per_rad = msg.kp;
        command.kd_nm_per_rad_s = msg.kd;
        command.torque_nm = msg.torque_nm;
        command_frame_.valid_mask = (UINT32_C(1) << axis);
        ++command_frame_.sequence;
        publish_command_frame();
    }

    void handle_aggregate_command(
        const hpm_gateway_msgs::msg::MotorMitCommandArray & msg)
    {
        command_frame_.valid_mask = msg.valid_mask;
        command_frame_.sequence = msg.sequence;
        std::memset(&command_frame_.output, 0,
                    sizeof(command_frame_.output));

        for (std::uint32_t axis = 0U; axis < kAxisCount; ++axis) {
            const auto & source = msg.commands[axis];
            const bool axis_is_valid =
                (msg.valid_mask & (UINT32_C(1) << axis)) != 0U;
            if (axis_is_valid &&
                (source.motor_id != (axis + 1U) ||
                 !is_finite(source.position_rad) ||
                 !is_finite(source.velocity_rad_s) ||
                 !is_finite(source.kp) ||
                 !is_finite(source.kd) ||
                 !is_finite(source.torque_nm))) {
                RCLCPP_ERROR(
                    get_logger(),
                    "Rejected aggregate command with non-finite axis %u",
                    axis);
                return;
            }

            auto & target = command_frame_.output.axis[axis];
            target.position_rad = source.position_rad;
            target.velocity_rad_s = source.velocity_rad_s;
            target.kp_nm_per_rad = source.kp;
            target.kd_nm_per_rad_s = source.kd;
            target.torque_nm = source.torque_nm;
        }
        publish_command_frame();
    }

    void publish_command_frame()
    {
        command_frame_.received_monotonic_us = monotonic_time_us();
        if (ecat_ && !ecat_->set_command(command_frame_)) {
            RCLCPP_WARN(get_logger(), "EtherCAT command buffer is full");
        }
    }

    /* Convert a CLOCK_MONOTONIC microsecond value to a wall-clock ROS stamp. */
    builtin_interfaces::msg::Time stamp_from_monotonic_us(
        std::uint64_t monotonic_us) const
    {
        const std::int64_t realtime_ns =
            static_cast<std::int64_t>(monotonic_us) * 1000 +
            monotonic_to_realtime_offset_ns_;
        builtin_interfaces::msg::Time stamp;
        stamp.sec = static_cast<std::int32_t>(realtime_ns / 1000000000LL);
        stamp.nanosec = static_cast<std::uint32_t>(realtime_ns % 1000000000LL);
        return stamp;
    }

    void publish_feedback()
    {
        if (!ecat_) {
            return;
        }

        gateway_rt_feedback_frame feedback{};
        if (!ecat_->get_feedback(feedback)) {
            return;
        }

        /* P3: stamp with the ECAT sample time, not the publish time. */
        const builtin_interfaces::msg::Time stamp =
            stamp_from_monotonic_us(feedback.received_monotonic_us);
        hpm_gateway_msgs::msg::MotorFeedbackArray message;
        message.stamp = stamp;
        /* P5: feedback_sequence is the ECAT-side generation counter. */
        message.sequence = feedback.feedback_sequence;
        message.valid_mask = UINT32_C(0xFFFF);

        for (std::uint32_t axis = 0U; axis < kAxisCount; ++axis) {
            const auto & source = feedback.input.axis[axis];
            auto & target = message.feedback[axis];
            target.stamp = stamp;
            target.motor_id = axis + 1U;
            target.position_rad = source.position_rad;
            target.velocity_rad_s = source.velocity_rad_s;
            target.torque_nm = source.torque_nm;
            target.status = 0U;
        }
        feedback_publisher_->publish(message);

        const auto & source_imu = feedback.input.imu;
        sensor_msgs::msg::Imu imu;
        imu.header.stamp = stamp;
        imu.header.frame_id = imu_frame_id_;
        const bool has_orientation =
            (source_imu.valid_fields & kImuFieldQuaternion) != 0U &&
            quaternion_is_valid(source_imu.quaternion);
        if (has_orientation) {
            const double q0 = source_imu.quaternion[0];
            const double q1 = source_imu.quaternion[1];
            const double q2 = source_imu.quaternion[2];
            const double q3 = source_imu.quaternion[3];
            const double norm = std::sqrt(
                q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
            imu.orientation.x = q1 / norm;
            imu.orientation.y = q2 / norm;
            imu.orientation.z = q3 / norm;
            imu.orientation.w = q0 / norm;
        } else {
            imu.orientation_covariance[0] = -1.0;
        }
        imu.linear_acceleration.x = source_imu.acceleration_mps2[0];
        imu.linear_acceleration.y = source_imu.acceleration_mps2[1];
        imu.linear_acceleration.z = source_imu.acceleration_mps2[2];
        imu.angular_velocity.x =
            source_imu.angular_rate_dps[0] * kDegreesToRadians;
        imu.angular_velocity.y =
            source_imu.angular_rate_dps[1] * kDegreesToRadians;
        imu.angular_velocity.z =
            source_imu.angular_rate_dps[2] * kDegreesToRadians;
        imu_publisher_->publish(imu);

        sensor_msgs::msg::Temperature temperature;
        temperature.header.stamp = stamp;
        temperature.header.frame_id = imu_frame_id_;
        temperature.temperature = source_imu.temperature_c;
        temperature.variance = -1.0;
        temperature_publisher_->publish(temperature);
    }

    void publish_status()
    {
        if (!ecat_) {
            RCLCPP_INFO(get_logger(), "EtherCAT disabled; ROS communication only");
            return;
        }

        const auto stats = ecat_->stats();
        RCLCPP_INFO(
            get_logger(),
            "cycles=%llu wkc_errors=%llu deadline_misses=%llu "
            "command_timeouts=%llu feedback_dropped=%llu "
            "max_exchange_us=%llu max_lateness_us=%llu",
            static_cast<unsigned long long>(stats.cycles),
            static_cast<unsigned long long>(stats.wkc_errors),
            static_cast<unsigned long long>(stats.deadline_misses),
            static_cast<unsigned long long>(stats.command_timeouts),
            static_cast<unsigned long long>(stats.feedback_dropped),
            static_cast<unsigned long long>(stats.max_exchange_us),
            static_cast<unsigned long long>(stats.max_lateness_us));
    }

    std::string command_topic_;
    std::string aggregate_command_topic_;
    std::string feedback_topic_;
    std::string imu_topic_;
    std::string temperature_topic_;
    std::string imu_frame_id_;
    std::string interface_name_;
    bool start_ethercat_{false};
    bool arm_all_axes_{false};
    int cycle_us_{1000};
    int rx_timeout_us_{500};
    int command_timeout_ms_{100};
    int rt_priority_{0};
    int rt_cpu_{-1};
    double feedback_publish_hz_{200.0};
    gateway_rt_command_frame command_frame_{};
    std::unique_ptr<hpm_gateway_control::GatewayEcat> ecat_;
    rclcpp::CallbackGroup::SharedPtr command_callback_group_;
    rclcpp::CallbackGroup::SharedPtr feedback_callback_group_;
    std::int64_t monotonic_to_realtime_offset_ns_{0};

    rclcpp::Publisher<hpm_gateway_msgs::msg::MotorFeedbackArray>::SharedPtr
        feedback_publisher_;
    rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_publisher_;
    rclcpp::Publisher<sensor_msgs::msg::Temperature>::SharedPtr
        temperature_publisher_;
    rclcpp::Subscription<hpm_gateway_msgs::msg::MotorMitCommand>::SharedPtr
        command_subscription_;
    rclcpp::Subscription<hpm_gateway_msgs::msg::MotorMitCommandArray>::SharedPtr
        aggregate_command_subscription_;
    rclcpp::TimerBase::SharedPtr feedback_timer_;
    rclcpp::TimerBase::SharedPtr status_timer_;
};

int main(int argc, char ** argv)
{
    rclcpp::init(argc, argv);
    try {
        auto node = std::make_shared<HpmGatewayControlNode>();
        rclcpp::executors::MultiThreadedExecutor executor(
            rclcpp::ExecutorOptions{}, 3U);
        executor.add_node(node);
        executor.spin();
    } catch (const std::exception & error) {
        std::fprintf(stderr, "hpm_gateway_control_node: %s\n", error.what());
        rclcpp::shutdown();
        return 1;
    }
    rclcpp::shutdown();
    return 0;
}
