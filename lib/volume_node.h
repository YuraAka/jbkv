#pragma once
#include "node.h"

namespace jbkv {

class VolumeNode : public Node<VolumeNode> {};

/// Creates empty volume
/// @return non-null volume ptr
VolumeNode::Ptr CreateVolume();
}  // namespace jbkv
