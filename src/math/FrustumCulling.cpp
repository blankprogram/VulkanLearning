
#include "engine/math/FrustumCulling.hpp"
#include <glm/gtc/matrix_access.hpp>
using namespace engine::math;

glm::vec4 FrustumCuller::normalizePlane(const glm::vec4 &p) {
    float invLen = 1.0f / glm::length(glm::vec3(p));
    return p * invLen;
}

void FrustumCuller::update(const glm::mat4 &m) {
    glm::mat4 t = glm::transpose(m);

    planes[0] = normalizePlane(t[3] + t[0]);
    planes[1] = normalizePlane(t[3] - t[0]);
    planes[2] = normalizePlane(t[3] + t[1]);
    planes[3] = normalizePlane(t[3] - t[1]);
    planes[4] = normalizePlane(t[3] + t[2]);
    planes[5] = normalizePlane(t[3] - t[2]);
}

bool FrustumCuller::isBoxVisible(const glm::vec3 &mn,
                                 const glm::vec3 &mx) const {
    for (int p = 0; p < 6; ++p) {
        const glm::vec4 &pl = planes[p];
        glm::vec3 positive = {pl.x >= 0 ? mx.x : mn.x, pl.y >= 0 ? mx.y : mn.y,
                              pl.z >= 0 ? mx.z : mn.z};
        if (glm::dot(glm::vec3(pl), positive) + pl.w < 0.0f) {
            return false;
        }
    }
    return true;
}
