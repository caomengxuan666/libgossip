# libgossip

## 语言索引

- [English](README.md)
- [中文版本](#中文版本)

<a id="中文版本"></a>
# libgossip

libgossip 是一个使用 C++17 实现的 Gossip 协议库，专为去中心化的分布式系统而设计。它提供了强大的节点发现、故障检测和元数据传播功能，注重可靠性和性能。

## 功能特性

- **Gossip 协议核心**：实现 SWIM（可扩展的弱一致性感染式进程组成员协议）协议，用于去中心化的节点成员管理
- **故障检测**：使用心跳机制和怀疑超时机制高效检测节点故障
- **元数据传播**：使用反熵 gossip 在集群中分发节点元数据
- **事件系统**：通知应用程序节点状态变化和元数据更新
- **模块化设计**：将核心协议实现与网络传输层分离
- **C API**：为非 C++ 应用提供 C 绑定
- **Python 绑定**：为 Python 应用提供易于集成的 Python 绑定

## 安装和集成

### 先决条件

- C++17 兼容编译器（GCC 7+，Clang 5+，MSVC 2017+）
- CMake 3.12+

### 从源码构建

```bash
git clone https://github.com/caomengxuan666/libgossip.git
cd libgossip
git submodule update --init  # 初始化 pybind11 子模块
mkdir build
cd build
cmake .. -DBUILD_PYTHON_BINDINGS=ON
cmake --build .
```

### 与 CMake 集成

您可以通过两种方式将 libgossip 集成到您的项目中：

1. 使用 find_package（安装后）：

```cmake
find_package(libgossip REQUIRED)
target_link_libraries(your_target libgossip::core)
# 对于网络功能：
target_link_libraries(your_target libgossip::network)
```

2. 使用 add_subdirectory（无需安装）：

```cmake
add_subdirectory(path/to/libgossip)
target_link_libraries(your_target libgossip::core)
# 对于网络功能：
target_link_libraries(your_target libgossip::network)
```

### 安装

要全局安装 libgossip：

```bash
mkdir build
cd build
cmake ..
cmake --build .
sudo cmake --install .
```

## 模块

### 核心模块

核心模块实现了 Gossip 协议逻辑，包括成员管理、故障检测和元数据传播。

- [gossip_core](include/core/gossip_core.hpp) - 主协议实现
- [node_view](include/core/gossip_core.hpp) - 带元数据的节点表示
- [gossip_message](include/core/gossip_core.hpp) - 用于网络传输的消息结构

### 网络模块

网络模块为 Gossip 协议提供传输层实现。

- [传输接口](include/net/udp_transport.hpp) - 抽象传输接口
- [UDP传输](include/net/udp_transport.hpp) - 基于 ASIO 的 UDP 传输实现
- [TCP传输](include/net/tcp_transport.hpp) - 基于 TCP 的可靠传输实现
- [传输工厂](include/net/transport_factory.hpp) - 用于创建传输实例的工厂
- [消息序列化器](include/net/udp_transport.hpp) - 抽象消息序列化接口
- [JSON序列化器](include/net/json_serializer.hpp) - 基于 JSON 的消息序列化实现

## 使用示例

请参阅[examples](examples/)目录获取详细的使用示例：

- [Simple Cluster](examples/simple_cluster.cpp) - 基础全网格集群设置
- [Seed-based Cluster](examples/seed_cluster.cpp) - 使用种子节点的真实部署
- [Advanced Cluster](examples/advanced_cluster.cpp) - 高级功能包括元数据和优雅离开
- [Simple Cluster (C API)](examples/simple_cluster_c.c) - 使用 C API 绑定
- [Network Example](examples/network_example.cpp) - 网络层使用示例

每个示例都演示了库使用的不同方面。

## API参考

### 核心类

- [gossip_core](include/core/gossip_core.hpp) - 主协议实现
- [node_view](include/core/gossip_core.hpp) - 带元数据的节点表示
- [gossip_message](include/core/gossip_core.hpp) - 用于网络传输的消息结构

### 网络类

- [transport](include/net/udp_transport.hpp) - 抽象传输接口
- [udp_transport](include/net/udp_transport.hpp) - 基于UDP的传输实现
- [tcp_transport](include/net/tcp_transport.hpp) - 基于TCP的传输实现
- [transport_factory](include/net/transport_factory.hpp) - 用于创建传输实例的工厂
- [message_serializer](include/net/udp_transport.hpp) - 抽象消息序列化接口
- [json_serializer](include/net/json_serializer.hpp) - 基于JSON的消息序列化实现

### C API函数

- `gossip_core_create()` - 创建一个新的gossip核心实例
- `gossip_core_destroy()` - 销毁gossip核心实例
- `gossip_core_tick()` - 驱动协议（应定期调用）
- `gossip_core_handle_message()` - 处理传入消息
- `gossip_core_meet()` - 将新节点引入集群
- `gossip_core_join()` - 加入现有节点
- `gossip_core_leave()` - 优雅地离开集群

### 关键方法

- `gossip_core::tick()` - 驱动协议（应定期调用）
- `gossip_core::handle_message()` - 处理传入消息
- `gossip_core::meet()` - 将新节点引入集群
- `gossip_core::join()` - 加入现有节点
- `gossip_core::leave()` - 优雅地离开集群

## 示例指南

详细示例在[examples](examples/)目录中提供：

1. **simple_cluster.cpp** - 展示具有全网格连接的基本集群形成
2. **seed_cluster.cpp** - 演示使用种子节点的真实部署
3. **advanced_cluster.cpp** - 展示高级功能如元数据和优雅离开
4. **simple_cluster_c.c** - 展示如何使用C API绑定
5. **network_example.cpp** - 展示如何使用网络层和不同传输协议

构建和运行示例：

```bash
cd build
cmake ..
cmake --build .
./examples/simple_cluster
./examples/seed_cluster
./examples/advanced_cluster
./examples/simple_cluster_c
./examples/network_example
```

## 发展路线图：从核心库到生产系统

libgossip-core已经是分布式系统的坚实基础。以下是未来的发展方向：

### 1. libgossip-net：网络传输层
核心层是网络无关的，现在我们需要具体的传输实现。

✅ **功能：**
- UDP传输：用于本地网络的轻量级不可靠传输
- TCP传输：用于广域网部署的可靠传输
- 序列化层：可插拔的消息序列化（JSON、FlatBuffers、Cap'n Proto）

### 2. libgossip-cluster：集群管理
构建更高层次的集群管理抽象。

✅ **功能：**
- 集群形成：自动化的集群创建和引导
- 节点发现：集群节点的服务发现
- 配置同步：分布式配置管理
- 领导者选举：简单的领导者选举机制

### 3. libgossip-utils：实用函数
为常见的分布式系统任务提供实用函数。

✅ **功能：**
- 时间同步：跨节点的时钟同步
- 负载均衡：在集群中分发工作负载
- 一致性哈希：用于分片的键分布
- 速率限制：控制高负载场景下的消息流

### 4. libgossip-monitoring：监控和可观察性
为生产部署提供监控和可观察性功能。

✅ **功能：**
- 指标收集：收集和暴露系统指标
- 健康检查：节点和集群健康监控
- 日志记录：具有不同级别的结构化日志
- 追踪：用于请求流的分布式追踪

### 5. 测试和基准测试
通过全面的测试确保可靠性和性能。

✅ **功能：**
- 单元测试：全面的单元测试覆盖
- 集成测试：端到端的集成测试
- 性能基准测试：定期的性能回归测试
- 混沌工程：用于弹性测试的故障注入

### 6. 文档和教程
创建全面的文档和学习资源。

✅ **功能：**
- API文档：包含示例的完整API参考
- 用户指南：常见用例的逐步指南
- 架构文档：系统架构的深入探讨
- 视频教程：可视化学习资源

### 7. 安全层
为生产环境提供安全保障。

✅ **功能：**
- TLS加密：保护传输安全
- 节点认证：基于证书或共享密钥
- 消息签名：防止伪造

### 8. 构建上层应用
用libgossip构建真实分布式系统。

✅ **项目构想：**
- Astra-CacheServer：分布式缓存,未来用libgossip重构这个项目的集群模块
- gossip-job：分布式任务调度，确保任务只被一个节点执行
- gossip-raft：在Gossip基础上实现Raft协议，用于强一致性

### 9. 跨平台与嵌入式
将libgossip移植到更多平台。

✅ **目标平台：**
- 嵌入式Linux（IoT设备）
- WebAssembly（浏览器内节点发现）
- 移动设备（Android/iOS P2P网络）

### 10. 开源与社区建设
将我们的成果分享给世界。

## 代码覆盖率

本项目包含代码覆盖率分析工具，以帮助确保质量和全面测试。

### 生成覆盖率报告

要生成代码覆盖率报告：

1. 配置启用覆盖率：
   ```bash
   mkdir build
   cd build
   cmake .. -DENABLE_COVERAGE=ON
   ```

2. 构建项目：
   ```bash
   cmake --build .
   ```

3. 运行测试：
   ```bash
   ctest -V
   ```

4. 生成覆盖率报告：
   ```bash
   ninja coverage  # 对于 Ninja 构建
   # 或
   make coverage   # 对于 Make 构建
   ```

5. 在浏览器中打开 `build/coverage_report_filtered/index.html` 查看 HTML 报告。

### 使用覆盖率脚本

或者，您可以使用提供的 Python 脚本生成过滤的覆盖率报告：

```bash
python script/generate_coverage.py --build-dir build
```

此脚本将生成一个报告，排除系统库、第三方代码和 Google Test 框架代码，
仅关注实际的项目代码。

### 覆盖率目标

项目目标：
- 行覆盖率：>90%
- 函数覆盖率：>90%

当前状态：97.5% 行覆盖率，97.5% 函数覆盖率（截至最新测试运行）。

## Python 绑定

libgossip 还提供 Python 绑定，便于与 Python 应用集成。

### 构建 Python 绑定

要构建 Python 绑定，您需要安装 pybind11：

```bash
# 克隆 pybind11 存储库作为子模块
git submodule update --init

# 启用 Python 绑定构建（默认）
mkdir build
cd build
cmake .. -DBUILD_PYTHON_BINDINGS=ON
cmake --build .
```

### 使用 Python 绑定

Python 使用示例：

```python
import libgossip

# 创建节点视图
node = libgossip.node_view()
node.ip = "127.0.0.1"
node.port = 8000

# 创建 gossip 核心实例
core = libgossip.gossip_core(node)

# 定期驱动协议
core.tick()
```

有关更多详细示例，请参阅 [Python 示例](bindings/python/examples/) 目录。

## 贡献

欢迎贡献！在提交拉取请求之前，请阅读我们的[贡献指南](CONTRIBUTING.md)。

## 许可证

本项目采用 MIT 许可证 - 有关详细信息，请参阅 [LICENSE](LICENSE) 文件。