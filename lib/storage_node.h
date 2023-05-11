#pragma once
#include "volume_node.h"
#include "noncopyable.h"
#include <string>
#include <vector>
#include <memory>

namespace jbkv {

/// Represents a virtual node that maps on some Volume nodes
/// Mapping is done either by mounting some volume nodes or by linking children
/// of mounted node
/// Structure of node has layered structure where bottom layers consists of
/// co-named children, and upper layers built from mount-points to current node
/// Layers are fixed at the moment of node materialization and stays constant
/// for all node life
/// Materialization is happening on node Create/Find/Mount methods
/// Mounts lives until explicit unmount (see below) and affect for future
/// materialization by the same path
class StorageNode : public Node<StorageNode> {
 public:
  /// @brief Mounts current node with subtree with root in given node
  /// @param node root of subtree to link
  /// @return new node with top-layer as given node, guaranteed to be non-null
  /// @note mount will be valid and globally visible for all result life
  /// @note mount does not make side-effects to current node, but does on future
  /// hierarchy browsing
  [[nodiscard]] virtual StorageNode::Ptr Mount(VolumeNode::Ptr node) = 0;

 public:
  [[nodiscard]] StorageNode::Ptr Mount(VolumeNode::List nodes) {
    if (nodes.empty()) {
      throw std::runtime_error("Unable to mount zero volumes");
    }

    StorageNode::Ptr result;
    for (auto&& node : std::move(nodes)) {
      result = result ? result->Mount(std::move(node)) : Mount(std::move(node));
    }

    return result;
  }
};

/// Mounts storage to given volume node
/// @return non-null storage-node
/// @note mount effect is for life-time of returned node
StorageNode::Ptr MountStorage(VolumeNode::Ptr node);
StorageNode::Ptr MountStorage(VolumeNode::List nodes);
}  // namespace jbkv
