#ifndef HPM_GATEWAY_CONTROL_GATEWAY_ECAT_API_HPP
#define HPM_GATEWAY_CONTROL_GATEWAY_ECAT_API_HPP

#include <cstdint>
#include <memory>
#include <string>

#include "hpm_gateway_control/gateway_motor_pdo.h"

namespace hpm_gateway_control {

struct GatewayEcatOptions final {
    std::string interface_name;
    std::uint32_t cycle_us{1000U};
    std::uint32_t rx_timeout_us{500U};
    std::uint32_t command_timeout_ms{100U};
    int rt_priority{0};
    int rt_cpu{-1};
    bool arm_all_axes{false};
};

struct GatewayEcatStats final {
    std::uint64_t cycles{0U};
    std::uint64_t wkc_errors{0U};
    std::uint64_t deadline_misses{0U};
    std::uint64_t command_timeouts{0U};
    std::uint64_t feedback_dropped{0U};
    std::uint64_t max_exchange_us{0U};
    std::uint64_t max_lateness_us{0U};
};

class GatewayEcat final {
public:
    explicit GatewayEcat(GatewayEcatOptions options);
    ~GatewayEcat();

    GatewayEcat(const GatewayEcat &) = delete;
    GatewayEcat & operator=(const GatewayEcat &) = delete;

    bool start();
    void stop() noexcept;
    bool is_running() const noexcept;

    bool set_command(const gateway_rt_command_frame & command) noexcept;
    bool get_feedback(gateway_rt_feedback_frame & feedback) noexcept;
    GatewayEcatStats stats() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace hpm_gateway_control

#endif
