#include <vulkan/vulkan.hpp>

#include <render/buffer.h>
#include <render/vulkan/device.hpp>

namespace my {
namespace render {
namespace vulkan {

class Buffer : public render::Buffer {
  public:
    // template<>
    Buffer(Device *device, const BuferType &type, const size_t &size)
        : render::Buffer(type, size), _device(device->_device) {

        vk::BufferCreateInfo create_info({}, size, usage,
                                         vk::SharingMode::eExclusive);
        this->_buffer = this->_device.createBufferUnique(create_info);

        auto mem_require =
            this->_device.getBufferMemoryRequirements(this->_buffer.get());
        vk::MemoryAllocateInfo alloc_info(
            mem_require.size, this->_vk_ctx->find_memory_type(
                                  mem_require.memoryTypeBits, properties));
        // auto buffer_memory = this->_device.allocateMemoryUnique(alloc_info);
        // this->_device.bindBufferMemory(buffer.get(), buffer_memory.get(), 0);
    }
    void copy_to_gpu() override {}

  private:
    vk::Device _device;
    vk::DeviceMemory _memory_map;
    vk::UniqueBuffer _buffer;
};

// template <class Res> class VkBuffer : public Buffer<Res> {
//   public:
//     VkBuffer(std::shared_ptr<Res> res) : Buffer<Res>(res) {}

//     template <typename T, typename = std::enable_if_t<
//                               std::is_base_of<VkBuffer<Res>, T>::value>>
//     static std::shared_ptr<T>
//     make(std::shared_ptr<Res> res,
//          const std::shared_ptr<RenderDevice> &device) {}
// };

// class VkTexture : VkBuffer<Texture> {
//   public:
//     VkTexture(std::shared_ptr<Texture> res, vulkan::RenderDevice *device)
//         : VkBuffer(res) {}
// };

} // namespace vulkan
} // namespace render
} // namespace my
