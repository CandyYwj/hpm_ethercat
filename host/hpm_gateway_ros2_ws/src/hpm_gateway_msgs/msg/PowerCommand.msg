# 电源板控制命令（PowerLink v1 SET_POWER / SET_POWER_ALL）。
# channel 取值：
#   0..9 = 单通道控制（配合 enable）
#   255  = 批量控制（忽略 enable，使用 state_mask 位图，bit n = 通道 n 目标状态）
builtin_interfaces/Time stamp
uint8 channel            # 0..9 单通道；255 = 批量
bool enable              # 单通道时使用：true=允许输出，false=禁止
uint16 state_mask        # 批量时使用：全部 10 位目标状态位图
