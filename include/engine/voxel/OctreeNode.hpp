
#pragma once
#include "Voxel.hpp"
#include <algorithm>
#include <array>
#include <memory>
#include <optional>

struct OctreeNode {
  bool isLeaf = false;
  std::array<std::unique_ptr<OctreeNode>, 8> children;
  std::optional<Voxel> voxel;

  bool isEmpty() const {
    return !isLeaf && std::all_of(children.begin(), children.end(),
                                  [](const auto &c) { return c == nullptr; });
  }
};
