# libgossip Examples

This directory contains example programs demonstrating various ways to use the libgossip library.

## simple_cluster.cpp

A basic example showing how to create a simple cluster with 3 nodes where each node knows about all other nodes directly. This demonstrates the fundamental gossip protocol functionality with full mesh connectivity.

## seed_cluster.cpp

An example demonstrating a more realistic deployment scenario using seed nodes. In this approach, only a few well-known seed nodes are explicitly configured, and other nodes discover each other through the gossip protocol. This is a common pattern in distributed systems.

## advanced_cluster.cpp

An advanced example showing additional features such as:
- Node metadata (roles, regions, datacenters, racks)
- Node graceful leave functionality
- Detailed statistics collection
- More complex event handling

---

# libgossip 示例

这个目录包含了演示如何使用 libgossip 库的各种示例程序。

## simple_cluster.cpp

一个基础示例，展示如何创建一个包含3个节点的简单集群，其中每个节点都直接知道所有其他节点。这演示了具有全网格连接的基本gossip协议功能。

## seed_cluster.cpp

一个演示使用种子节点的更实际部署场景的示例。在这种方法中，只显式配置少数几个众所周知的种子节点，其他节点通过gossip协议相互发现。这是分布式系统中的常见模式。

## advanced_cluster.cpp

一个展示附加功能的高级示例，包括：
- 节点元数据（角色、区域、数据中心、机架）
- 节点优雅离开功能
- 详细的统计信息收集
- 更复杂的事件处理