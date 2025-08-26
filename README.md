# libgossip

## Language Index 语言索引

- [English](#english-version)
- [中文版本](#中文版本)

<!-- English Version -->
<a id="english-version"></a>
# libgossip

libgossip is a C++17 implementation of the Gossip protocol, designed for decentralized distributed systems. It provides robust node discovery, failure detection, and metadata propagation with an emphasis on reliability and performance.

## Features

- **Gossip Protocol Core**: Implements SWIM (Scalable Weakly-consistent Infection-style process group Membership) protocol for decentralized node membership management
- **Failure Detection**: Efficiently detects node failures using heartbeat mechanism with suspicion timeouts
- **Metadata Propagation**: Distributes node metadata across the cluster using anti-entropy gossip
- **Event System**: Notifies applications of node status changes and metadata updates
- **Header-only Library**: Easy integration with minimal dependencies
- **C API**: Provides C bindings for non-C++ applications
- **Python Bindings**: Provides Python bindings for easy integration with Python applications

## Installation and Integration

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.12+

### Building from Source

```bash
git clone https://github.com/caomengxuan666/libgossip.git
cd libgossip
git submodule update --init  # Initialize pybind11 submodule
mkdir build
cd build
cmake .. -DBUILD_PYTHON_BINDINGS=ON
cmake --build .
```

### Integration with CMake

You can integrate libgossip into your project in two ways:

1. Using find_package (after installation):

```cmake
find_package(libgossip REQUIRED)
target_link_libraries(your_target libgossip::core)
```

2. Using add_subdirectory (without installation):

```cmake
add_subdirectory(path/to/libgossip)
target_link_libraries(your_target libgossip::core)
```

### Installation

To install libgossip system-wide:

```bash
mkdir build
cd build
cmake ..
cmake --build .
sudo cmake --install .
```

## Usage Examples

See the [examples](examples/) directory for detailed usage examples:

Python examples are available in the [bindings/python](bindings/python/) directory:

- [example.py](bindings/python/example.py) - Basic single node example
- [two_node_example.py](bindings/python/two_node_example.py) - Two-node interaction example
- [debug_example.py](bindings/python/debug_example.py) - Debug information example

- [Simple Cluster](examples/simple_cluster.cpp) - Basic full-mesh cluster setup
- [Seed-based Cluster](examples/seed_cluster.cpp) - Real-world deployment using seed nodes
- [Advanced Cluster](examples/advanced_cluster.cpp) - Advanced features including metadata and graceful leave
- [Simple Cluster (C API)](examples/simple_cluster_c.c) - Using the C API bindings

Each example demonstrates different aspects of the library usage patterns.

## API Reference

### Core Classes

- [gossip_core](include/core/gossip_core.hpp) - Main protocol implementation
- [node_view](include/core/gossip_core.hpp) - Node representation with metadata
- [gossip_message](include/core/gossip_core.hpp) - Message structure for network transport

### Python API Classes

- `gossip.GossipCore` - Main protocol implementation
- `gossip.NodeView` - Node representation with metadata
- `gossip.GossipMessage` - Message structure for network transport
- `gossip.NodeId` - Node unique identifier
- `gossip.NodeStatus` - Node status enumeration
- `gossip.MessageType` - Message type enumeration

### C API Functions

- `gossip_core_create()` - Create a new gossip core instance
- `gossip_core_destroy()` - Destroy a gossip core instance
- `gossip_core_tick()` - Drive the protocol (should be called periodically)
- `gossip_core_handle_message()` - Process incoming messages
- `gossip_core_meet()` - Introduce a new node to the cluster
- `gossip_core_join()` - Join an existing node
- `gossip_core_leave()` - Gracefully leave the cluster

### Key Methods

- `gossip_core::tick()` - Drive the protocol (should be called periodically)
- `gossip_core::handle_message()` - Process incoming messages
- `gossip_core::meet()` - Introduce a new node to the cluster
- `gossip_core::join()` - Join an existing node
- `gossip_core::leave()` - Gracefully leave the cluster

## Examples Guide

Detailed examples are provided in the [examples](examples/) directory:

1. **simple_cluster.cpp** - Shows basic cluster formation with full mesh connectivity
2. **seed_cluster.cpp** - Demonstrates realistic deployment using seed nodes
3. **advanced_cluster.cpp** - Illustrates advanced features like metadata and graceful leave
4. **simple_cluster_c.c** - Shows how to use the C API bindings

To build and run examples:

```bash
cd build
cmake .. -DBUILD_PYTHON_BINDINGS=ON
cmake --build .
./examples/simple_cluster
./examples/seed_cluster
./examples/advanced_cluster
./examples/simple_cluster_c
python ../bindings/python/example.py
python ../bindings/python/two_node_example.py
python ../bindings/python/debug_example.py
```

```bash
cd build
cmake ..
cmake --build .
./examples/simple_cluster
./examples/seed_cluster
./examples/advanced_cluster
./examples/simple_cluster_c
```

## Roadmap: From Core Library to Production System

libgossip-core is already a solid foundation for distributed systems. Here are the directions for future development:

### 1. libgossip-net: Network Transport Layer
The core layer is network-agnostic, now we need concrete transport implementations.

✅ **UDP Implementation (lightweight, low latency)**
- Using boost::asio or native sockets
- Support multicast (optional) for fast discovery
- Implement message retransmission (optional, for critical messages)

✅ **TCP Implementation (reliable, ordered)**
- For large messages or state synchronization
- Implement connection management

✅ **Example:**
```
auto transport = std::make_shared<UdpTransport>("127.0.0.1", 8000);
transport->set_gossip_core(gossip_core);
transport->start();
```

### 2. Serialization Layer
Enable gossip_message to be transmitted across different languages and platforms.

✅ **Options:**
- FlatBuffers: Zero-copy, high-performance, cross-language
- Cap'n Proto: Similar to FlatBuffers, Google-made
- Custom binary format: Ultra-compact, suitable for embedded systems

✅ **Example:**
```cpp
auto buffer = serializer::pack(message);
send(buffer.data(), buffer.size());

// On receiving end
auto msg = serializer::unpack(received_data, size);
gossip_core.handle_message(*msg, now);
```

### 3. libgossip-cluster: Cluster Management Layer
Build higher-level cluster functions on top of gossip-core.

✅ **Features:**
- Member management: add_node, remove_node
- Failover: When primary node goes offline, automatically elect new primary
- Sharding routing: Key routing based on consistent hashing
- Configuration synchronization: Propagate cluster configuration

✅ **Can be used to build:**
- Redis Cluster-like key-value storage
- Distributed cache
- Service discovery system

### 4. Service Discovery
Use libgossip as a service registry and discovery system.

✅ **Extend node_view:**
```cpp
struct service_info {
    std::string name;
    std::string version;
    std::map<std::string, std::string> tags;
    std::map<std::string, std::string> meta;
};
```

✅ **Usage:**
```cpp
// Service registration
gossip_core.register_service("user-service", "v1.0", "region", "us-east-1");

// Service discovery
auto services = gossip_core.get_services("user-service");
```

This is similar to the core functionality of Consul.

### 5. Configuration Management and Distribution
Use Gossip's anti-entropy feature to distribute configuration.

✅ **Scenarios:**
- Dynamic log level updates
- Feature toggles
- Hot updates of algorithm parameters

✅ **Advantages:**
- No central configuration server
- High availability, eventual consistency

### 6. Monitoring and Observability
Provide operational support for production environments.

✅ **Features:**
- Metrics: Expose Prometheus metrics
  - gossip_nodes_count
  - gossip_messages_sent_total
  - gossip_messages_received_total
  - gossip_network_latency_ms
- Tracing: Integrate with OpenTelemetry
- Logging: Structured log output

### 7. Security Layer
Provide security guarantees for production environments.

✅ **Features:**
- TLS encryption: Protect transport security
- Node authentication: Based on certificates or shared keys
- Message signing: Prevent forgery

### 8. Higher-level Applications
Build real distributed systems using libgossip.

✅ **Project Ideas:**
- Astra-CacheServer: we would refactor the existing cache server to use libgossip for node discovery and fault tolerance
- gossip-job: Distributed task scheduling, ensuring tasks are executed by only one node
- gossip-raft: Implement Raft protocol on top of Gossip for strong consistency

### 9. Cross-platform and Embedded Support
Port libgossip to more platforms.

✅ **Target Platforms:**
- Embedded Linux (IoT devices)
- WebAssembly (browser-based node discovery)
- Mobile devices (Android/iOS P2P networks)

### 10. Open Source and Community Building
Share your achievements with the world.

## Python Bindings

libgossip also provides Python bindings for easy integration with Python applications.

### Building Python Bindings

To build the Python bindings, you need to have pybind11 installed:

```bash
# Clone the pybind11 repository as a submodule
git submodule update --init

# Build with Python bindings enabled (default)
mkdir build
cd build
cmake .. -DBUILD_PYTHON_BINDINGS=ON
cmake --build .
```

### Using Python Bindings

```python
import gossip

# Create a node view for ourself
self_node = gossip.NodeView()
self_node.ip = "127.0.0.1"
self_node.port = 7000
self_node.status = gossip.NodeStatus.ONLINE

# Define callbacks
def send_callback(msg, target):
    print(f"Sending message of type {msg.type} to {target.ip}:{target.port}")

def event_callback(node, old_status):
    print(f"Node {node.ip}:{node.port} changed from {old_status} to {node.status}")

# Initialize gossip core
core = gossip.GossipCore(self_node, send_callback, event_callback)

# Meet another node
other_node = gossip.NodeView()
other_node.ip = "127.0.0.1"
other_node.port = 7001
other_node.status = gossip.NodeStatus.JOINING

core.meet(other_node)

# Run gossip protocol
for i in range(5):
    core.tick()
    time.sleep(0.1)
```

See [bindings/python/example.py](bindings/python/example.py) for a complete example.

<!-- 中文版本 -->
<a id="中文版本"></a>
# libgossip

libgossip是一个使用C++17实现的Gossip协议库，专为去中心化的分布式系统而设计。它提供了强大的节点发现、故障检测和元数据传播功能，注重可靠性和性能。

## 功能特性

- **Gossip协议核心**：实现了SWIM（可扩展的弱一致性感染式进程组成员协议）用于去中心化的节点成员管理
- **故障检测**：使用心跳机制和怀疑超时机制高效检测节点故障
- **元数据传播**：使用反熵gossip在集群中分发节点元数据
- **事件系统**：在节点状态变化和元数据更新时通知应用程序
- **仅头文件库**：易于集成，依赖项少
- **C API**：为非C++应用程序提供C语言绑定

## 安装和集成

### 先决条件

- C++17兼容编译器（GCC 7+，Clang 5+，MSVC 2017+）
- CMake 3.12+

### 从源码构建

```
git clone https://github.com/caomengxuan666/libgossip.git
cd libgossip
mkdir build
cd build
cmake ..
cmake --build .
```

### 与CMake集成

有两种方式将libgossip集成到您的项目中：

1. 使用find_package（安装后）：

```
find_package(libgossip REQUIRED)
target_link_libraries(your_target libgossip::core)
```

2. 使用add_subdirectory（无需安装）：

```
add_subdirectory(path/to/libgossip)
target_link_libraries(your_target libgossip::core)
```

### 安装

要将libgossip安装到系统中：

```
mkdir build
cd build
cmake ..
cmake --build .
sudo cmake --install .
```

## 使用示例

请参阅[examples](examples/)目录获取详细的使用示例：

- [Simple Cluster](examples/simple_cluster.cpp) - 基础全网格集群设置
- [Seed-based Cluster](examples/seed_cluster.cpp) - 使用种子节点的真实部署
- [Advanced Cluster](examples/advanced_cluster.cpp) - 高级功能包括元数据和优雅离开
- [Simple Cluster (C API)](examples/simple_cluster_c.c) - 使用C API绑定

每个示例都演示了库使用的不同方面。

## API参考

### 核心类

- [gossip_core](include/core/gossip_core.hpp) - 主协议实现
- [node_view](include/core/gossip_core.hpp) - 带元数据的节点表示
- [gossip_message](include/core/gossip_core.hpp) - 用于网络传输的消息结构

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

构建和运行示例：

```
cd build
cmake ..
cmake --build .
./examples/simple_cluster
./examples/seed_cluster
./examples/advanced_cluster
./examples/simple_cluster_c
```

## 发展路线图：从核心库到生产系统

libgossip-core已经是分布式系统的坚实基础。以下是未来的发展方向：

### 1. libgossip-net：网络传输层
核心层是网络无关的，现在我们需要具体的传输实现。

✅ **UDP实现（轻量、低延迟）**
- 使用boost::asio或原生socket
- 支持多播（可选）用于快速发现
- 实现消息重传（可选，用于关键消息）

✅ **TCP实现（可靠、有序）**
- 用于大消息或状态同步
- 实现连接管理

✅ **示例：**
```
auto transport = std::make_shared<UdpTransport>("127.0.0.1", 8000);
transport->set_gossip_core(gossip_core);
transport->start();
```

### 2. 实现序列化层（Serialization）
让gossip_message能在不同语言、平台间传输。

✅ **方案选择：**
- FlatBuffers：零拷贝，高性能，跨语言
- Cap'n Proto：类似FlatBuffers，Google出品
- 自定义二进制格式：极致紧凑，适合嵌入式

✅ **示例：**
```
auto buffer = serializer::pack(message);
send(buffer.data(), buffer.size());

// 接收端
auto msg = serializer::unpack(received_data, size);
gossip_core.handle_message(*msg, now);
```

### 3. 构建libgossip-cluster：集群管理层
在gossip-core之上，构建更高级的集群功能。

✅ **功能：**
- 成员管理：add_node, remove_node
- 故障转移：当主节点下线，自动选举新主
- 分片路由：基于一致性哈希的键路由
- 配置同步：传播集群配置

✅ **可用于构建：**
- 类Redis Cluster的键值存储
- 分布式缓存
- 服务发现系统

### 4. 实现服务发现（Service Discovery）
将libgossip用作服务注册与发现。

✅ **扩展node_view：**
```
struct service_info {
    std::string name;
    std::string version;
    std::map<std::string, std::string> tags;
    std::map<std::string, std::string> meta;
};
```

✅ **用法：**
```
// 服务注册
gossip_core.register_service("user-service", "v1.0", "region", "us-east-1");

// 服务发现
auto services = gossip_core.get_services("user-service");
```

这类似于Consul的核心功能。

### 5. 实现配置管理与发布
利用Gossip的反熵特性，分发配置。

✅ **场景：**
- 动态更新日志级别
- 开关功能（Feature Toggle）
- 算法参数热更新

✅ **优势：**
- 无中心配置服务器
- 高可用、最终一致

### 6. 实现监控与可观测性
为生产环境提供运维支持。

✅ **功能：**
- Metrics：暴露Prometheus指标
  - gossip_nodes_count
  - gossip_messages_sent_total
  - gossip_messages_received_total
  - gossip_network_latency_ms
- Tracing：集成OpenTelemetry
- Logging：结构化日志输出

### 7. 实现安全层（Security）
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

## Python 绑定

`libgossip` 还提供了 Python 绑定，便于与 Python 应用程序轻松集成。

### 构建 Python 绑定

要构建 Python 绑定，你需要先安装 `pybind11`：

```bash
# 将 pybind11 仓库作为子模块克隆
git submodule update --init

# 启用 Python 绑定进行构建（默认开启）
mkdir build
cd build
cmake .. -DBUILD_PYTHON_BINDINGS=ON
cmake --build .