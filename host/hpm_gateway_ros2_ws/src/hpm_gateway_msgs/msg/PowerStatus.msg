# 电源板遥测快照（PowerLink v1 STATUS / 0x83，主机侧解析后发布）。
# 通道映射（与固件/硬件一致，见 docs/40）：
#   0..5 = 12V_1,12V_2,12V_FAN,12V_4,5V,12V_3
#   6..9 = 48V_M4,48V_M1,48V_M2,48V_M3（注意 48V 顺序与协议文档初稿不同，已按硬件修正）
builtin_interfaces/Time stamp
uint32 battery_mv        # VBAT，单位 mV
int16 ntc1_deci_c        # NTC1 温度，单位 0.1°C；-32768 = 无效
int16 ntc2_deci_c        # NTC2 温度，单位 0.1°C；-32768 = 无效
uint16 output_state      # bit n = 通道 n 当前状态（1=允许输出）
uint8 alarms             # bit0=LOW_VOLTAGE，bit1=COMM_LOST
bool comm_lost           # 主机侧链路看门狗（>3s 无合法帧）
uint8 last_ack_cmd       # 最近一次 CMD_ACK 回显的命令码，0 = 无
uint8 last_ack_status    # 最近一次 CMD_ACK 的状态码（见 PowerLinkStatus）
bool last_ack_ok         # 最近一次命令是否成功（status == OK）
