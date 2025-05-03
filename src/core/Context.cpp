
#include "engine/core/Context.hpp"

namespace engine {

Context::Context() : context_() {}

vk::raii::Context &Context::get() { return context_; }

} // namespace engine
