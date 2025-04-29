
#include "engine/resources/Model.hpp"
#include "thirdparty/tiny_gltf.h"
#include "thirdparty/tiny_obj_loader.h"
#include <stdexcept>

Model::Model(VkDevice device, VkPhysicalDevice physicalDevice,
             VkCommandPool commandPool, VkQueue graphicsQueue,
             const std::string &path, Texture *texture)
    : device_(device), physicalDevice_(physicalDevice),
      commandPool_(commandPool), graphicsQueue_(graphicsQueue),
      texture_(texture) { // <-- store texture pointer

  if (path.size() >= 4 && (path.substr(path.size() - 4) == ".obj" ||
                           path.substr(path.size() - 4) == ".OBJ"))
    loadOBJ(path);
  else if ((path.size() >= 5 && path.substr(path.size() - 5) == ".gltf") ||
           (path.size() >= 4 && path.substr(path.size() - 4) == ".glb"))
    loadGLTF(path);
  else
    throw std::runtime_error("Unsupported model format: " + path);
}

void Model::loadOBJ(const std::string &path) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  std::string basedir = path.substr(0, path.find_last_of("/\\") + 1);

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(),
                        basedir.c_str()))
    throw std::runtime_error("Failed to load OBJ: " + warn + err);

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  for (const auto &shape : shapes) {
    for (const auto &idx : shape.mesh.indices) {
      Vertex v{};
      v.pos[0] = attrib.vertices[3 * idx.vertex_index + 0];
      v.pos[1] = attrib.vertices[3 * idx.vertex_index + 1];
      v.pos[2] = attrib.vertices[3 * idx.vertex_index + 2];

      if (idx.texcoord_index >= 0) {
        v.uv[0] = attrib.texcoords[2 * idx.texcoord_index + 0];
        v.uv[1] = 1.0f - attrib.texcoords[2 * idx.texcoord_index + 1];
      }
      if (idx.normal_index >= 0) {
        v.normal[0] = attrib.normals[3 * idx.normal_index + 0];
        v.normal[1] = attrib.normals[3 * idx.normal_index + 1];
        v.normal[2] = attrib.normals[3 * idx.normal_index + 2];
      }

      vertices.push_back(v);
      indices.push_back(uint32_t(indices.size()));
    }
  }

  mesh_ = std::make_unique<Mesh>(device_, physicalDevice_, commandPool_,
                                 graphicsQueue_, vertices, indices);

  // 🚀 Load the texture automatically

  if (!materials.empty()) {
    std::string texFile = materials[0].diffuse_texname;
    if (!texFile.empty()) {
      std::string texturePath = basedir + texFile;
      texture_->Load(device_, physicalDevice_, commandPool_, graphicsQueue_,
                     texturePath);
    }
  }
}

void Model::loadGLTF(const std::string &path) {
  tinygltf::Model gltfModel;
  tinygltf::TinyGLTF loader;
  std::string err, warn;
  if (!loader.LoadASCIIFromFile(&gltfModel, &err, &warn, path))
    throw std::runtime_error("Failed to load GLTF: " + warn + err);

  const auto &mesh = gltfModel.meshes[0];
  const auto &primitive = mesh.primitives[0];

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  const auto &posAcc =
      gltfModel.accessors[primitive.attributes.find("POSITION")->second];
  const auto &posBufView = gltfModel.bufferViews[posAcc.bufferView];
  const auto &posBuf = gltfModel.buffers[posBufView.buffer];
  const float *positions = reinterpret_cast<const float *>(
      &posBuf.data[posBufView.byteOffset + posAcc.byteOffset]);

  const auto &normalAcc =
      gltfModel.accessors[primitive.attributes.find("NORMAL")->second];
  const auto &normalBufView = gltfModel.bufferViews[normalAcc.bufferView];
  const auto &normalBuf = gltfModel.buffers[normalBufView.buffer];
  const float *normals = reinterpret_cast<const float *>(
      &normalBuf.data[normalBufView.byteOffset + normalAcc.byteOffset]);

  const auto &uvAcc =
      gltfModel.accessors[primitive.attributes.find("TEXCOORD_0")->second];
  const auto &uvBufView = gltfModel.bufferViews[uvAcc.bufferView];
  const auto &uvBuf = gltfModel.buffers[uvBufView.buffer];
  const float *uvs = reinterpret_cast<const float *>(
      &uvBuf.data[uvBufView.byteOffset + uvAcc.byteOffset]);

  for (size_t i = 0; i < posAcc.count; ++i) {
    Vertex v{};
    v.pos[0] = positions[i * 3 + 0];
    v.pos[1] = positions[i * 3 + 1];
    v.pos[2] = positions[i * 3 + 2];

    v.normal[0] = normals[i * 3 + 0];
    v.normal[1] = normals[i * 3 + 1];
    v.normal[2] = normals[i * 3 + 2];

    v.uv[0] = uvs[i * 2 + 0];
    v.uv[1] = uvs[i * 2 + 1];

    vertices.push_back(v);
  }

  const auto &idxAcc = gltfModel.accessors[primitive.indices];
  const auto &idxBufView = gltfModel.bufferViews[idxAcc.bufferView];
  const auto &idxBuf = gltfModel.buffers[idxBufView.buffer];
  const uint16_t *idxs = reinterpret_cast<const uint16_t *>(
      &idxBuf.data[idxBufView.byteOffset + idxAcc.byteOffset]);

  for (size_t i = 0; i < idxAcc.count; ++i) {
    indices.push_back(uint32_t(idxs[i]));
  }

  mesh_ = std::make_unique<Mesh>(device_, physicalDevice_, commandPool_,
                                 graphicsQueue_, vertices, indices);
}
