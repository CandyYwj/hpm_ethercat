# 串口电机遥测协议

固件状态任务每秒发送一个 16 轴快照。协议使用 ASCII、`CRLF` 行结束，便于
USB-UART 工具直接观察，也能在丢字节后通过下一条 `MT` 头重新同步。

```text
MT,1,<uptime_ms>,<command_sequence>\r\n
MD,1,<motor_id>,<online>,<enabled>,<fault_hex>,<position_rad>,<velocity_rad_s>,<torque_nm>,<temperature_c>\r\n
```

每个快照固定包含 `motor_id=1..16` 各一条 `MD`。`online` 使用 250 ms 反馈超时
判定；未接线或超时轴仍会输出，但值是最近一次采样值（初始为 0）。温度为
电机 CAN 反馈中的温度字段；协议不支持温度的电机输出 0。

上位机工具为 `scripts/motor_telemetry_serial.py`。运行时只记录完整快照到 JSONL，
不启动绘图，避免串口采集受绘图刷新影响：

```bash
python3 scripts/motor_telemetry_serial.py record \
  --port /dev/ttyUSB0 --baud 115200 \
  --output recordings/motors.jsonl --duration-seconds 300
```

采集结束后离线渲染。每个电机生成一张 PNG，图内依次包含位置、速度、力矩和
温度四个子图，共生成 `motor_01.png` 到 `motor_16.png`。绘图需要安装可选依赖
`matplotlib`：

```bash
python3 -m pip install matplotlib
python3 scripts/motor_telemetry_serial.py render \
  --input recordings/motors.jsonl --output-dir recordings/plots
```

`--duration-seconds` 可省略，省略时按 `Ctrl+C` 结束记录。记录文件是一行一个
完整 16 轴快照，便于后续重复渲染或接入其它分析程序。

## 波特率核算

当前一帧约 16 条 `MD`（约 75--105 字节/条，取决于数值位数）加头部，按最坏
`1800` 字节/秒估算。8N1 串口每字节占 10 bit，因此需要约 `18,000 bit/s`。
`115200` baud 的有效吞吐约 `11,520 byte/s`，占用约 16%，余量充足；即使保留
少量启动/故障日志也不会逼近上限。无需为了该 1 Hz 遥测提高波特率。若恢复
高频调试打印或改为 10 Hz，建议使用 230400 baud，并限制其它 `printf`。

控制台 UART 的实际波特率由板级 BSP 配置决定；`UART12 460800` 是 IMU 专用
串口，不是本遥测输出串口。上位机必须与调试串口实际配置保持一致。
