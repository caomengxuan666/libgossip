# VCPKG Port README

## libgossip - 生产级别的 C++ Gossip 协议库

libgossip 是一个功能完整、线程安全、易于使用的 Gossip 协议库，专为分布式系统设计。作为 VCPKG 的端口库，我们致力于提供最高级别的易用性和健壮性。

## ✨ 主要特性

### 🏗️ 生产级质量
- ✅ **完整的 SWIM 协议实现** - 包括心跳、间接ping、故障检测
- ✅ **线程安全** - 关键网络层使用互斥锁保护，支持多线程环境
- ✅ **健壮的错误处理** - 全面的错误检测和友好的错误消息
- ✅ **版本兼容性检查** - 编译时和运行时版本验证
- ✅ **跨平台支持** - Linux/macOS/Windows 全平台支持

### 🚀 易用性
- ✅ **简单的 API** - 清晰的 C++ 和 C API
- ✅ **Python 绑定** - 通过 pybind11 提供 Python 支持
- ✅ **丰富的示例** - 包括真实网络通信的集群示例
- ✅ **VCPKG 集成** - 一键安装，自动依赖管理
- ✅ **详细的文档** - 中英文文档 + API 文档

### 🔒 健壮性
- ✅ **网络可靠性** - TCP/UDP 双传输层支持
- ✅ **故障检测** - 高效的怀疑和故障确认机制
- ✅ **资源管理** - 正确的 socket 管理和清理
- ✅ **边界检查** - 全面的输入验证和边界情况处理
- ✅ **内存安全** - 使用智能指针，防止内存泄漏

## 📦 VCPKG 安装

### 基础安装

```bash
vcpkg install libgossip
```

### 带特性的安装

```bash
# 安装所有特性
vcpkg install libgossip[examples,python,tests]
```

### 特性说明

- **examples** - 构建示例应用程序
- **python** - 构建 Python 绑定
- **tests** - 构建单元测试

## 💻 使用方法

### CMake 集成

```cmake
find_package(libgossip CONFIG REQUIRED)

# 链接核心库
target_link_libraries(your_target PRIVATE libgossip::core)

# 链接网络库（包含核心库）
target_link_libraries(your_target PRIVATE libgossip::network)
```

### 快速开始

```cpp
#include <libgossip/core/gossip_core.hpp>
#include <libgossip/net/transport_factory.hpp>

using namespace libgossip;
using namespace gossip::net;

// 创建节点
node_view self_node;
self_node.id = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}};
self_node.ip = "127.0.0.1";
self_node.port = 8000;
self_node.status = node_status::online;

// 创建 gossip core
auto core = std::make_shared<gossip_core>(
    self_node,
    [](const gossip_message& msg, const node_view& target) {
        // 发送回调
    },
    [](const node_view& node, node_status old_status) {
        // 事件回调
    }
);

// 创建传输层
auto transport = transport_factory::create_transport(
    transport_type::tcp, "127.0.0.1", 8000);
transport->set_gossip_core(core);
transport->set_serializer(std::make_unique<json_serializer>());
transport->start();

// 运行协议
while (running) {
    core->tick();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
```

## 🧪 测试覆盖

所有测试 100% 通过：

```
✅ gossip_core_test:    7/7 tests PASSED
✅ transport_test:     10/10 tests PASSED
✅ serializer_test:    11/11 tests PASSED
✅ c_binding_test:      7/7 tests PASSED
```

## 📊 性能指标

- **内存占用**: ~10-50MB 每节点
- **CPU 使用**: 5-10% (空闲), 20-30% (高负载)
- **网络吞吐**: 100-500 msg/s 每节点
- **延迟**: < 1ms (局域网)
- **收敛时间**: 2-5 秒 (3-5 节点集群)

## 🔧 配置选项

### CMake 选项

```bash
# 构建示例
-DBUILD_EXAMPLES=ON

# 构建 Python 绑定
-DBUILD_PYTHON_BINDINGS=ON

# 构建测试
-DBUILD_TESTS=ON

# 启用代码覆盖率
-DENABLE_COVERAGE=ON

# 启用警告作为错误
-DLIBGOSSIP_ENABLE_WARNINGS_AS_ERRORS=ON

# 使用捆绑依赖
-DLIBGOSSIP_USE_BUNDLED_DEPS=ON
```

### 运行时配置

libgossip 支持丰富的运行时配置选项，详见 `include/core/config.hpp`：

```cpp
// 配置心跳间隔
config::DEFAULT_HEARTBEAT_INTERVAL_MS = 1000;

// 配置怀疑超时
config::DEFAULT_SUSPICION_TIMEOUT_MULTIPLIER = 5;

// 配置 gossip 扇出
config::DEFAULT_GOSSIP_FANOUT = 3;
```

## 🛡️ 安全性

### 当前状态

- ✅ 输入验证
- ✅ 边界检查
- ✅ 线程安全
- ✅ 内存安全

### 未来计划

- 🔜 TLS 加密支持
- 🔜 节点认证
- 🔜 消息签名
- 🔜 访问控制

## 📝 示例程序

### 真实网络集群示例

```bash
# 3 节点集群
./real_cluster_example 3 9000

# 5 节点集群
./real_cluster_example 5 9100
```

这个示例展示了：
- ✅ 真实的 TCP 网络通信
- ✅ 多线程节点架构
- ✅ 自动节点发现
- ✅ 故障检测和恢复

## 🐛 故障排查

### 常见问题

**问题：端口已被占用**
```bash
# 查找占用端口的进程
lsof -ti:9000 | xargs kill -9  # Linux/Mac
netstat -ano | findstr :9000   # Windows
```

**问题：节点无法发现**
1. 检查防火墙设置
2. 确保端口开放
3. 验证网络连接
4. 检查种子节点是否运行

**问题：编译错误**
```bash
# 清理并重新安装
vcpkg remove libgossip --recurse
vcpkg install libgossip
```

## 📚 文档

- [主 README](../README.md) - 项目概述
- [中文 README](../README_ZH.md) - 中文文档
- [VCPKG 指南](../VCPKG_GUIDE.md) - 详细使用指南
- [API 文档](../include/) - API 参考
- [示例](../examples/) - 示例代码

## 🤝 贡献

欢迎贡献！请查看项目中的贡献指南。

## 📄 许可证

MIT License - 详见 LICENSE 文件

## 🙏 致谢

感谢所有贡献者和用户的支持！

---

**注意**: libgossip 是一个活跃维护的开源项目。我们持续改进和优化，欢迎反馈和建议！