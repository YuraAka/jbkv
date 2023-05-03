
class FsSaver final : public VolumeNode::Reader {
 public:
  explicit FsSaver(const std::filesystem::path& path) {
    std::cout << "Saving to " << path << std::endl;
  }

  void OnLink(const NodeName& parent, const NodeName& child) override {
    std::cout << "Saving parent-child: " << parent << "->" << child
              << std::endl;
  }

  void OnData(const NodeName& name, const NodeKey& key,
              const Value& value) override {
    std::cout << "- saving node '" << name << "', " << key << " = " << value
              << std::endl;
  }
};

class FsLoader final : public VolumeNode::Writer {
 public:
  explicit FsLoader(const std::filesystem::path& path)
      : hierarchy_{{"root", {"d1", "d2"}}, {"d1", {"d3", "d4"}}},
        data_{{"d1", {{"k1", "v1"}, {"k2", "v2"}}},
              {"d4", {{"k3", "v3"}, {"k4", "v4"}}}} {
    std::cout << "Loading from " << path << std::endl;
  }

  void OnData(const NodeName& self, const DataCallback& cb) {
    auto dit = data_.find(self);
    if (dit != data_.end()) {
      for (auto&& [key, value] : dit->second) {
        Value val(std::move(value));
        cb(std::move(key), std::move(val));
      }
    }
  }

  void OnLink(const NodeName& self, const LinkCallback& cb) {
    auto hit = hierarchy_.find(self);
    if (hit != hierarchy_.end()) {
      for (auto&& child_name : hit->second) {
        cb(std::move(child_name));
      }
    }
  }

 private:
  using NodeHierarchy = std::vector<std::string>;
  using Hierarchy = std::unordered_map<std::string, NodeHierarchy>;
  using KvList = std::vector<std::pair<std::string, std::string>>;
  using Data = std::unordered_map<std::string, KvList>;

  Hierarchy hierarchy_;
  Data data_;
};

template <typename NodeType, typename U>
void Accept(const std::shared_ptr<NodeType>& root, U& visitor) {
  std::deque<typename NodeType::Ptr> nodes{root};
  while (!nodes.empty()) {
    auto node = nodes.front();
    nodes.pop_front();
    node->Accept(visitor);

    node->AcceptChildren([&nodes](const auto& name, const auto& node) {
      nodes.push_back(node);
    });
  }
}

void jbkv::Save(const Volume& volume, const std::filesystem::path& path) {
  FsSaver saver(path);
  Accept(volume.Root(), saver);
}

void jbkv::Load(Volume& volume, const std::filesystem::path& path) {
  FsLoader loader(path);
  Accept(volume.Root(), loader);
}
