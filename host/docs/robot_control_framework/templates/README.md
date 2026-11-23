# 配置模板

模板用于新协议/设备评审，不是可直接加载的生产配置。复制后必须经过 schema 校验、设备身份 allowlist、总线 admission、签名和 HIL 验证。实时进程只读取编译后的 profile，不解析这些 YAML。

- [protocol-spec.template.yaml](protocol-spec.template.yaml)：线缆协议、版本、帧和错误语义。
- [device-manifest.template.yaml](device-manifest.template.yaml)：设备实例、端点、能力、限值、时序和 ROS 映射。

提交新模板时同时提交 golden frames、配置 hash、测试结果和回滚说明。
