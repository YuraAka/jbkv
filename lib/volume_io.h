#pragma once
#include <filesystem>
#include <iostream>
#include "volume_node.h"

namespace jbkv {

/// Serialization/deserialization
/// @{
void Save(const VolumeNode::Ptr& root, std::ostream& stream);
void Load(const VolumeNode::Ptr& root, std::istream& stream);

void Save(const VolumeNode::Ptr& root, const std::filesystem::path& path);
void Load(const VolumeNode::Ptr& root, const std::filesystem::path& path);
/// @}

}  // namespace jbkv
