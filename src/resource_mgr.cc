#include "resource_mgr.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace my {
Texture::Texture(const std::string &path) : Resource(path) {
    int channels;
    this->pixels = stbi_load(
        path.c_str(), reinterpret_cast<int *>(&this->width),
        reinterpret_cast<int *>(&this->height), &channels, STBI_rgb_alpha);
    if (!this->pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
}

Texture::~Texture() { stbi_image_free(this->pixels); }

std::unique_ptr<ResourceMgr> ResourceMgr::create(AsyncTask *task) {
    return std::make_unique<ResourceMgr>(task);
}

} // namespace my
