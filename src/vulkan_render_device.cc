#include <vulkan/vulkan.hpp>

#include "logger.h"
#include "render_device.h"
#include "vulkan_ctx.h"

namespace {
class VulkanRenderDevice : public my::RenderDevice {
  public:
    explicit VulkanRenderDevice(my::VulkanCtx *ctx)
        : _vk_ctx(ctx), _device(ctx->get_device()),
          _primary_cmd_buffer(this->_create_primary_buffer()) {

        if (this->_enable_sample_buffer) {
            GLOG_D("enable sample buffer");
            this->_sample_buffer = this->_create_sample_buffer();
        }

        if (this->_enable_depth_buffer) {
            GLOG_D("enable depth buffer");
            this->_depth_buffer = this->_create_depth_buffer();
        }

        this->_create_descriptor_set_layout();
        this->_create_desciptor_pool();
        this->_create_descriptor_sets();

        this->_render_pass = this->_create_render_pass();
    }

    ~VulkanRenderDevice() override { this->_device.waitIdle(); }

    void draw_begin() override {
        auto &swapchain = this->_vk_ctx->get_swapchain();
        auto &current_buffer_index = this->_vk_ctx->get_current_buffer_index();
        auto &current_buffer = this->_frame_buffers[current_buffer_index].get();

        auto &cb = this->_primary_cmd_buffer.get();
        cb.begin({vk::CommandBufferUsageFlagBits::eSimultaneousUse});

        std::vector<vk::ClearValue> clear_values;
        clear_values.push_back(
            vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}));

        if (this->_enable_depth_buffer) {
            clear_values.push_back(vk::ClearDepthStencilValue(1.0f, 0));
        }

        vk::RenderPassBeginInfo render_pass_info(
            this->_render_pass.get(), current_buffer,
            {{0, 0}, swapchain.extent}, clear_values.size(),
            clear_values.data());

        cb.beginRenderPass(render_pass_info, vk::SubpassContents::eInline);

        std::vector<vk::DescriptorSet> descriptor_set = {
            this->_descriptor_sets[current_buffer_index].get()};
        cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                              this->_pipeline_layout.get(), 0, descriptor_set,
                              {});
    }

    void draw_end() override {
        auto &cb = this->_primary_cmd_buffer.get();
        cb.endRenderPass();
        cb.end();
        this->_vk_ctx->sumit_command_buffer(cb);
    }

    void with_draw(const vk::RenderPass &render_pass,
                   const my::VulkanDrawCallback &draw_func) override {}

    void bind_pipeline(const vk::Pipeline &pipeline) override {
        auto &cb = this->_primary_cmd_buffer.get();
        cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    };

    void bind_vertex_buffer(
        const std::shared_ptr<my::BufferBinding<vk::UniqueBuffer>>
            &buffer_binding) override {
        auto &cb = this->_primary_cmd_buffer.get();
        vk::DeviceSize offsets[] = {0};
        std::vector<vk::Buffer> vertex_buffers = {buffer_binding->buffer.get()};
        cb.bindVertexBuffers(VERTEX_BINDING_ID, vertex_buffers.size(),
                             vertex_buffers.data(), offsets);
    }

    void
    bind_index_buffer(const std::shared_ptr<my::BufferBinding<vk::UniqueBuffer>>
                          &buffer_binding) override {
        auto &cb = this->_primary_cmd_buffer.get();
        cb.bindIndexBuffer(buffer_binding->buffer.get(), 0,
                           vk::IndexType::eUint32);
    }

    void push_constants() override {
        auto &cb = this->_primary_cmd_buffer.get();
        cb.pushConstants(this->_pipeline_layout, , uint32_t offset, ArrayProxy<const T> values)
    }

    void draw(size_t index_count) override {
        auto &cb = this->_primary_cmd_buffer.get();
        cb.drawIndexed(index_count, 1, 0, 0, 0);
    }

    std::shared_ptr<my::VulkanShader>
    create_shader(const std::vector<char> &code,
                  const vk::ShaderStageFlagBits &stage,
                  const std::string &name) override {
        return std::make_shared<my::VulkanShader>(this->_device, code, stage,
                                                  name);
    }

    // TODO: swapchain dep
    vk::UniquePipeline create_pipeline(
        const std::vector<std::shared_ptr<my::VulkanShader>> &shaders,
        const my::VertexDesciption &vertex_desc,
        const std::vector<vk::PushConstantRange> &push_constant_ranges)
        override {
        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;
        std::transform(shaders.begin(), shaders.end(),
                       std::back_inserter(shader_stages),
                       [](const std::shared_ptr<my::VulkanShader> &shader) {
                           return shader->get();
                       });

        vk::PipelineVertexInputStateCreateInfo vertex_input_state(
            {}, vertex_desc.bindings.size(), vertex_desc.bindings.data(),
            vertex_desc.attributes.size(), vertex_desc.attributes.data());

        vk::PipelineInputAssemblyStateCreateInfo input_assembly_state(
            {}, vk::PrimitiveTopology::eTriangleList, VK_FALSE);

        auto &swapchain = this->_vk_ctx->get_swapchain();
        vk::Viewport viewport(0.0f, 0.0f, swapchain.extent.width,
                              swapchain.extent.height, 0.0f, 1.0f);
        vk::Rect2D scissor({0, 0}, swapchain.extent);
        vk::PipelineViewportStateCreateInfo viewport_state({}, 1, &viewport, 1,
                                                           &scissor);

        // XXX: front face
        vk::PipelineRasterizationStateCreateInfo rasterization_state(
            {}, VK_FALSE, VK_FALSE, vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise, VK_FALSE,
            {}, {}, {}, 1.0f);

        vk::PipelineMultisampleStateCreateInfo multisample_state(
            {}, this->_msaa_samples, VK_FALSE);

        vk::PipelineColorBlendAttachmentState color_blend_attachment_state(
            VK_FALSE);
        color_blend_attachment_state.setColorWriteMask(
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

        vk::PipelineColorBlendStateCreateInfo color_blend_state(
            {}, VK_FALSE, vk::LogicOp::eCopy, 1, &color_blend_attachment_state,
            {0.0f, 0.0f, 0.0f, 0.0f});

        this->_pipeline_layout = this->_device.createPipelineLayoutUnique(
            vk::PipelineLayoutCreateInfo(
                {}, 1, &this->_descriptor_set_layout.get(),
                push_constant_ranges.size(), push_constant_ranges.data()));

        vk::GraphicsPipelineCreateInfo create_info(
            {}, shader_stages.size(), shader_stages.data(), &vertex_input_state,
            &input_assembly_state, {}, &viewport_state, &rasterization_state,
            &multisample_state, {}, &color_blend_state, {},
            this->_pipeline_layout.get(), this->_render_pass.get(), 0, {}, {});

        vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;
        if (this->_enable_depth_buffer) {
            depth_stencil_state = {
                {}, VK_TRUE, VK_TRUE, vk::CompareOp::eLess, VK_FALSE, VK_FALSE};
            create_info.setPDepthStencilState(&depth_stencil_state);
        }

        return this->_device.createGraphicsPipelineUnique({}, create_info);
    }

    virtual std::shared_ptr<my::BufferBinding<vk::UniqueBuffer>>
    create_buffer(const void *data, const vk::DeviceSize &buffer_size,
                  const vk::BufferUsageFlags &usage) override {
        return this->_create_buffer_with_device(data, buffer_size, usage);
    }

    std::shared_ptr<my::BufferBinding<vk::UniqueBuffer>>
    create_vertex_buffer(const std::vector<my::Vertex> &vertices) override {
        vk::DeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();
        return this->_create_buffer_with_device(
            vertices.data(), buffer_size,
            vk::BufferUsageFlagBits::eVertexBuffer);
    }

    std::shared_ptr<my::BufferBinding<vk::UniqueBuffer>>
    create_index_buffer(const std::vector<uint32_t> &indices) override {
        vk::DeviceSize buffer_size = sizeof(indices[0]) * indices.size();
        return this->_create_buffer_with_device(
            indices.data(), buffer_size, vk::BufferUsageFlagBits::eIndexBuffer);
    }

    my::VulkanUniformBuffer
    create_uniform_buffer(const vk::DeviceSize &buffer_size) override {
        auto buffer_binding = this->_create_buffer(
            buffer_size, vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent);

        vk::DescriptorBufferInfo buffer_info(buffer_binding->buffer.get(), 0,
                                             buffer_size);

        for (auto i = 0; i < this->_descriptor_sets.size(); i++) {
            std::vector<vk::WriteDescriptorSet> desc_writes = {
                vk::WriteDescriptorSet(this->_descriptor_sets[i].get(),
                                       BindingLayoutIndex::UNIFORM_BUFFER, 0, 1,
                                       vk::DescriptorType::eUniformBuffer, {},
                                       &buffer_info)};
            this->_device.updateDescriptorSets(desc_writes, {});
        }
        return {buffer_binding, buffer_size};
    }
    my::VulkanImageBuffer create_texture_buffer(void *pixels, size_t w,
                                                size_t h) override {
        vk::DeviceSize img_size = w * h * 4;
        auto staging_buffer_binding = this->_create_buffer(
            img_size, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent);

        auto data = this->_device.mapMemory(
            staging_buffer_binding->memory.get(), 0, img_size);
        ::memcpy(data, pixels, img_size);
        this->_device.unmapMemory(staging_buffer_binding->memory.get());

        uint32_t mip_levels = this->_enable_mipmaps
                                  ? std::floor(std::log2(std::max(w, h))) + 1
                                  : 1;
        GLOG_D("mip levels %d", mip_levels);

        auto buffer_binding = this->_create_image_buffer(
            w, h, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransferSrc |
                vk::ImageUsageFlagBits::eTransferDst |
                vk::ImageUsageFlagBits::eSampled,
            vk::MemoryPropertyFlagBits::eDeviceLocal, mip_levels);

        this->_transition_image_layout(buffer_binding->buffer.get(),
                                       vk::Format::eR8G8B8A8Unorm, mip_levels,
                                       vk::ImageLayout::eUndefined,
                                       vk::ImageLayout::eTransferDstOptimal);

        this->_copy_buffer_to_image(staging_buffer_binding->buffer.get(),
                                    buffer_binding->buffer.get(), w, h);

        if (this->_enable_mipmaps) {
            this->_generate_mipmaps(buffer_binding->buffer.get(),
                                    vk::Format::eR8G8B8A8Unorm, w, h,
                                    mip_levels);
        } else {
            this->_transition_image_layout(
                buffer_binding->buffer.get(), vk::Format::eR8G8B8A8Unorm,
                mip_levels, vk::ImageLayout::eTransferDstOptimal,
                vk::ImageLayout::eShaderReadOnlyOptimal);
        }

        auto image_view = this->_create_image_view(
            buffer_binding->buffer.get(), vk::Format::eR8G8B8A8Unorm,
            vk::ImageAspectFlagBits::eColor, mip_levels);

        auto tex_sampler = this->_create_texture_sampler(mip_levels);

        vk::DescriptorImageInfo img_info(
            tex_sampler.get(), image_view.get(),
            vk::ImageLayout::eShaderReadOnlyOptimal);

        for (auto i = 0; i < this->_descriptor_sets.size(); i++) {
            std::vector<vk::WriteDescriptorSet> desc_writes = {
                vk::WriteDescriptorSet(
                    this->_descriptor_sets[i].get(),
                    BindingLayoutIndex::COMBINED_IMAGE_SAMPLER, 0, 1,
                    vk::DescriptorType::eCombinedImageSampler, &img_info)};

            this->_device.updateDescriptorSets(desc_writes, {});
        }

        return {std::move(buffer_binding), std::move(image_view),
                std::move(tex_sampler)};
    }

    void
    copy_to_buffer(void *src, size_t size,
                   const my::BufferBinding<vk::UniqueBuffer> &bind) override {
        void *dst = this->_device.mapMemory(bind.memory.get(), 0, size);
        ::memcpy(dst, src, size);
        this->_device.unmapMemory(bind.memory.get());
    }

    void wait_idle() override { this->_device.waitIdle(); };

  private:
    my::VulkanCtx *_vk_ctx;
    vk::Device _device;
    vk::UniqueCommandBuffer _primary_cmd_buffer;
    my::VulkanDrawCtx _draw_ctx;

    vk::UniquePipelineLayout _pipeline_layout;

    enum BindingLayoutIndex { UNIFORM_BUFFER = 0, COMBINED_IMAGE_SAMPLER = 1 };
    vk::UniqueDescriptorSetLayout _descriptor_set_layout;
    std::vector<vk::DescriptorSetLayoutBinding> _descriptor_set_layout_binding;
    vk::UniqueDescriptorPool _descriptor_pool;
    std::vector<vk::UniqueDescriptorSet> _descriptor_sets;

    //
    my::VulkanImageBuffer _depth_buffer;

    my::VulkanImageBuffer _sample_buffer;
    vk::SampleCountFlagBits _msaa_samples = vk::SampleCountFlagBits::e1;

    std::vector<vk::UniqueImageView> _swapchain_image_views;

    vk::UniqueRenderPass _render_pass;
    std::vector<vk::UniqueFramebuffer> _frame_buffers;

    // Options
    // bool _enable_depth_buffer = true;
    // bool _enable_sample_buffer = true;
    // bool _enable_mipmaps = true;

    bool _enable_depth_buffer = true;
    bool _enable_sample_buffer = false;
    bool _enable_mipmaps = false;

    vk::UniqueCommandBuffer _create_primary_buffer() {
        // auto count = this->_vk_ctx->get_swapchain().image_count;
        auto &cmd_pool = this->_vk_ctx->get_command_pool(
            my::VulkanCommandPoolType::VK_COMMAND_POOL_COMMON);

        vk::CommandBufferAllocateInfo allocate_info(
            cmd_pool, vk::CommandBufferLevel::ePrimary, 1);

        return std::move(
            this->_device.allocateCommandBuffersUnique(allocate_info).at(0));
    }

    my::VulkanImageBuffer _create_sample_buffer() {
        auto &swapchain = this->_vk_ctx->get_swapchain();
        vk::Format format = swapchain.format;
        this->_msaa_samples = this->_vk_ctx->get_max_usable_sample_count();
        GLOG_D("msaa samples %s", vk::to_string(this->_msaa_samples).c_str());

        auto buffer_binding = this->_create_image_buffer(
            swapchain.extent.width, swapchain.extent.height, format,
            vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment,
            vk::MemoryPropertyFlagBits::eDeviceLocal, 1, this->_msaa_samples);

        auto sample_image_view =
            this->_create_image_view(buffer_binding->buffer.get(), format,
                                     vk::ImageAspectFlagBits::eColor);

        return {std::move(buffer_binding), std::move(sample_image_view)};
    }

    my::VulkanImageBuffer _create_depth_buffer() {
        vk::Format depth_format = this->_vk_ctx->get_depth_format();
        auto &swapchain = this->_vk_ctx->get_swapchain();

        auto buffer_binding = this->_create_image_buffer(
            swapchain.extent.width, swapchain.extent.height, depth_format,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eDepthStencilAttachment,
            vk::MemoryPropertyFlagBits::eDeviceLocal, 1, this->_msaa_samples);

        auto depth_image_view =
            this->_create_image_view(buffer_binding->buffer.get(), depth_format,
                                     vk::ImageAspectFlagBits::eDepth);

        return {std::move(buffer_binding), std::move(depth_image_view)};
    }

    // TODO: swapchain dep
    std::vector<vk::UniqueFramebuffer>
    _create_frame_buffers(const vk::RenderPass &render_pass) {
        auto &swapchain = this->_vk_ctx->get_swapchain();

        this->_swapchain_image_views.clear();
        std::transform(swapchain.images.begin(), swapchain.images.end(),
                       std::back_inserter(this->_swapchain_image_views),
                       [this, &swapchain](const vk::Image &img) {
                           return this->_create_image_view(
                               img, swapchain.format,
                               vk::ImageAspectFlagBits::eColor);
                       });

        std::vector<vk::UniqueFramebuffer> frame_buffers;
        std::transform(
            this->_swapchain_image_views.begin(),
            this->_swapchain_image_views.end(),
            std::back_inserter(frame_buffers),
            [this, &render_pass,
             &swapchain](const vk::UniqueImageView &img_view) {
                std::vector<vk::ImageView> attachments;

                if (this->_enable_sample_buffer) {
                    attachments.push_back(
                        this->_sample_buffer.image_view.get());
                } else {
                    attachments.push_back(img_view.get());
                }

                if (this->_enable_depth_buffer) {
                    attachments.push_back(this->_depth_buffer.image_view.get());
                }

                if (this->_enable_sample_buffer) {
                    attachments.push_back(img_view.get());
                }

                vk::FramebufferCreateInfo create_info(
                    {}, render_pass, attachments.size(), attachments.data(),
                    swapchain.extent.width, swapchain.extent.height, 1);
                return this->_device.createFramebufferUnique(create_info);
            });

        return frame_buffers;
    }

    std::shared_ptr<my::BufferBinding<vk::UniqueBuffer>>
    _create_buffer(vk::DeviceSize size, const vk::BufferUsageFlags &usage,
                   const vk::MemoryPropertyFlags &properties) {
        vk::BufferCreateInfo create_info({}, size, usage,
                                         vk::SharingMode::eExclusive);
        auto buffer = this->_device.createBufferUnique(create_info);

        auto mem_require =
            this->_device.getBufferMemoryRequirements(buffer.get());
        vk::MemoryAllocateInfo alloc_info(
            mem_require.size, this->_vk_ctx->find_memory_type(
                                  mem_require.memoryTypeBits, properties));
        auto buffer_memory = this->_device.allocateMemoryUnique(alloc_info);
        this->_device.bindBufferMemory(buffer.get(), buffer_memory.get(), 0);

        return std::make_shared<my::BufferBinding<vk::UniqueBuffer>>(
            std::move(buffer), std::move(buffer_memory));
    }

    std::shared_ptr<my::BufferBinding<vk::UniqueBuffer>>
    _create_buffer_with_device(const void *src,
                               const vk::DeviceSize &buffer_size,
                               const vk::BufferUsageFlags &usage) {
        auto staging_buffer_binding = this->_create_buffer(
            buffer_size, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent);

        auto data = this->_device.mapMemory(
            staging_buffer_binding->memory.get(), 0, buffer_size);
        ::memcpy(data, src, buffer_size);
        this->_device.unmapMemory(staging_buffer_binding->memory.get());

        auto buffer_binding = this->_create_buffer(
            buffer_size, vk::BufferUsageFlagBits::eTransferDst | usage,
            vk::MemoryPropertyFlagBits::eDeviceLocal);

        this->_copy_buffer(staging_buffer_binding->buffer.get(),
                           buffer_binding->buffer.get(), buffer_size);
        return buffer_binding;
    }

    std::shared_ptr<my::BufferBinding<vk::UniqueImage>> _create_image_buffer(
        uint32_t width, uint32_t height, const vk::Format &format,
        const vk::ImageTiling &tiling, const vk::ImageUsageFlags &usage,
        const vk::MemoryPropertyFlags &properties, uint32_t mip_levels = 1,
        const vk::SampleCountFlagBits &num_samples =
            vk::SampleCountFlagBits::e1) {

        vk::ImageCreateInfo create_info(
            {}, vk::ImageType::e2D, format, {width, height, 1}, mip_levels, 1,
            num_samples, tiling, usage, vk::SharingMode::eExclusive);

        auto image = this->_device.createImageUnique(create_info);

        auto mem_require =
            this->_device.getImageMemoryRequirements(image.get());
        vk::MemoryAllocateInfo alloc_info(
            mem_require.size, this->_vk_ctx->find_memory_type(
                                  mem_require.memoryTypeBits, properties));
        auto buffer_memory = this->_device.allocateMemoryUnique(alloc_info);
        this->_device.bindImageMemory(image.get(), buffer_memory.get(), 0);

        return std::make_shared<my::BufferBinding<vk::UniqueImage>>(
            std::move(image), std::move(buffer_memory));
    }

    void _single_time_commands(
        const vk::CommandPool &command_pool, const my::VulkanQueueType &type,
        const std::function<void(vk::CommandBuffer &)> &commands) {
        vk::CommandBufferAllocateInfo alloc_info(
            command_pool, vk::CommandBufferLevel::ePrimary, 1);

        auto command_buffer = std::move(
            this->_device.allocateCommandBuffersUnique(alloc_info).front());

        command_buffer->begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        commands(command_buffer.get());
        command_buffer->end();

        vk::SubmitInfo submit_info;
        submit_info.setCommandBufferCount(1);
        submit_info.setPCommandBuffers(&command_buffer.get());

        this->_vk_ctx->get_queue(type).submit({submit_info}, {});
        this->_vk_ctx->get_queue(type).waitIdle();
    }

    void _copy_buffer(vk::Buffer &src_buffer, vk::Buffer &dst_buffer,
                      const vk::DeviceSize &size) {
        this->_single_time_commands(
            this->_vk_ctx->get_command_pool(my::VK_COMMAND_POOL_MEM),
            my::VK_QUEUE_TRANSFER, [&](vk::CommandBuffer &command_buffer) {
                vk::BufferCopy copy_region({}, {}, size);
                command_buffer.copyBuffer(src_buffer, dst_buffer, 1,
                                          &copy_region);
            });
    }

    void _transition_image_layout(vk::Image &image, const vk::Format &format,
                                  uint32_t mip_levels,
                                  const vk::ImageLayout &old_layout,
                                  const vk::ImageLayout &new_layout) {
        this->_single_time_commands(
            this->_vk_ctx->get_command_pool(my::VK_COMMAND_POOL_MEM),
            my::VK_QUEUE_TRANSFER, [&](vk::CommandBuffer &command_buffer) {
                vk::PipelineStageFlags source_stage;
                vk::PipelineStageFlags destination_stage;

                vk::ImageMemoryBarrier barrier(
                    {}, {}, old_layout, new_layout, VK_QUEUE_FAMILY_IGNORED,
                    VK_QUEUE_FAMILY_IGNORED, image,
                    {vk::ImageAspectFlagBits::eColor, 0, mip_levels, 0, 1});

                if (old_layout == vk::ImageLayout::eUndefined &&
                    new_layout == vk::ImageLayout::eTransferDstOptimal) {
                    barrier.setSrcAccessMask({});
                    barrier.setDstAccessMask(
                        vk::AccessFlagBits::eTransferWrite);
                    source_stage = vk::PipelineStageFlagBits::eTopOfPipe;
                    destination_stage = vk::PipelineStageFlagBits::eTransfer;
                } else if (old_layout == vk::ImageLayout::eTransferDstOptimal &&
                           new_layout ==
                               vk::ImageLayout::eShaderReadOnlyOptimal) {
                    barrier.setSrcAccessMask(
                        vk::AccessFlagBits::eTransferWrite);
                    barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
                    source_stage = vk::PipelineStageFlagBits::eTransfer;
                    destination_stage =
                        vk::PipelineStageFlagBits::eFragmentShader;
                } else {
                    throw std::invalid_argument(
                        "unsupported layout transition!");
                }

                command_buffer.pipelineBarrier(source_stage, destination_stage,
                                               {}, {}, {}, {barrier});
            });
    }

    void _copy_buffer_to_image(vk::Buffer &buffer, vk::Image &image,
                               uint32_t width, uint32_t height) {
        this->_single_time_commands(
            this->_vk_ctx->get_command_pool(my::VK_COMMAND_POOL_MEM),
            my::VK_QUEUE_TRANSFER, [&](vk::CommandBuffer &command_buffer) {
                vk::BufferImageCopy region(
                    0, 0, 0, {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                    {0, 0, 0}, {width, height, 1});
                command_buffer.copyBufferToImage(
                    buffer, image, vk::ImageLayout::eTransferDstOptimal, 1,
                    &region);
            });
    }

    vk::UniqueImageView
    _create_image_view(const vk::Image &image, const vk::Format &format,
                       const vk::ImageAspectFlags &aspect_flags,
                       uint32_t mip_levels = 1) {
        vk::ImageViewCreateInfo create_info;
        create_info.setImage(image);
        create_info.setViewType(vk::ImageViewType::e2D);
        create_info.setFormat(format);
        // create_info.setComponents({});
        create_info.setSubresourceRange({aspect_flags, 0, mip_levels, 0, 1});
        return this->_device.createImageViewUnique(create_info);
    }

    void _generate_mipmaps(const vk::Image &image, vk::Format format,
                           uint32_t w, uint32_t h, uint32_t mip_levels) {

        auto physical_device = this->_vk_ctx->get_physical_device();

        if (!(physical_device.getFormatProperties(format)
                  .optimalTilingFeatures &
              vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
            throw std::runtime_error(
                "texture image format does not support linear blitting!");
        }

        this->_single_time_commands(
            this->_vk_ctx->get_command_pool(my::VK_COMMAND_POOL_MEM),
            my::VK_QUEUE_TRANSFER, [&](vk::CommandBuffer &command_buffer) {
                vk::ImageMemoryBarrier barrier;
                barrier.setImage(image);
                barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
                barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
                barrier.setSubresourceRange(
                    {vk::ImageAspectFlagBits::eColor, {}, 1, 0, 1});

                int32_t mip_w = w;
                int32_t mip_h = h;

                for (uint32_t i = 1; i < mip_levels; ++i) {
                    barrier.subresourceRange.setBaseMipLevel(i - 1);
                    barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
                    barrier.setNewLayout(vk::ImageLayout::eTransferSrcOptimal);
                    barrier.setSrcAccessMask(
                        vk::AccessFlagBits::eTransferWrite);
                    barrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);

                    command_buffer.pipelineBarrier(
                        vk::PipelineStageFlagBits::eTransfer,
                        vk::PipelineStageFlagBits::eTransfer, {}, {}, {},
                        {barrier});

                    vk::ImageBlit blit;
                    blit.setSrcOffsets(
                        {vk::Offset3D{0, 0, 0}, vk::Offset3D{mip_w, mip_h, 1}});
                    blit.setSrcSubresource(
                        {vk::ImageAspectFlagBits::eColor, i - 1, 0, 1});
                    blit.setDstOffsets(
                        {vk::Offset3D{0, 0, 0},
                         vk::Offset3D{mip_w > 1 ? mip_w / 2 : 1,
                                      mip_h > 1 ? mip_h / 2 : 1, 1}});
                    blit.setDstSubresource(
                        {vk::ImageAspectFlagBits::eColor, i, 0, 1});

                    command_buffer.blitImage(
                        image, vk::ImageLayout::eTransferSrcOptimal, image,
                        vk::ImageLayout::eTransferDstOptimal, {blit},
                        vk::Filter::eLinear);

                    barrier.setOldLayout(vk::ImageLayout::eTransferSrcOptimal);
                    barrier.setNewLayout(
                        vk::ImageLayout::eShaderReadOnlyOptimal);
                    barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferRead);
                    barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
                    command_buffer.pipelineBarrier(
                        vk::PipelineStageFlagBits::eTransfer,
                        vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {},
                        {barrier});

                    if (mip_w > 1)
                        mip_w /= 2;
                    if (mip_h > 1)
                        mip_h /= 2;
                }

                barrier.subresourceRange.setBaseMipLevel(mip_levels - 1);
                barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
                barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
                barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
                barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

                command_buffer.pipelineBarrier(
                    vk::PipelineStageFlagBits::eTransfer,
                    vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {},
                    {barrier});
            });
    }

    vk::UniqueSampler _create_texture_sampler(uint32_t mip_leves) {
        vk::SamplerCreateInfo create_info(
            {}, vk::Filter::eLinear, vk::Filter::eLinear,
            vk::SamplerMipmapMode::eLinear, vk::SamplerAddressMode::eRepeat,
            vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
            0.0f, VK_FALSE, 1, VK_FALSE, vk::CompareOp::eAlways, 0.0f,
            static_cast<float>(mip_leves), vk::BorderColor::eIntOpaqueBlack,
            VK_FALSE);
        return this->_device.createSamplerUnique(create_info);
    }

    void _create_descriptor_set_layout() {
        // TODO:
        vk::DescriptorSetLayoutBinding uniform_layout_binding(
            BindingLayoutIndex::UNIFORM_BUFFER,
            vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eVertex, {});
        vk::DescriptorSetLayoutBinding sampler_layout_binding(
            BindingLayoutIndex::COMBINED_IMAGE_SAMPLER,
            vk::DescriptorType::eCombinedImageSampler, 1,
            vk::ShaderStageFlagBits::eFragment, {});
        this->_descriptor_set_layout_binding = {uniform_layout_binding,
                                                sampler_layout_binding};

        vk::DescriptorSetLayoutCreateInfo create_info(
            {}, this->_descriptor_set_layout_binding.size(),
            this->_descriptor_set_layout_binding.data());

        this->_descriptor_set_layout =
            this->_device.createDescriptorSetLayoutUnique(create_info);
    }

    void _create_desciptor_pool() {
        std::vector<vk::DescriptorPoolSize> pool_sizes;
        auto img_count = this->_vk_ctx->get_swapchain().image_count;
        std::transform(
            this->_descriptor_set_layout_binding.begin(),
            this->_descriptor_set_layout_binding.end(),
            std::back_inserter(pool_sizes),
            [img_count](const vk::DescriptorSetLayoutBinding &binding) {
                return vk::DescriptorPoolSize(binding.descriptorType,
                                              img_count);
            });

        vk::DescriptorPoolCreateInfo create_info(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, img_count,
            pool_sizes.size(), pool_sizes.data());

        this->_descriptor_pool =
            this->_device.createDescriptorPoolUnique(create_info);
    }

    void _create_descriptor_sets() {
        auto img_count = this->_vk_ctx->get_swapchain().image_count;
        std::vector<vk::DescriptorSetLayout> layouts(
            img_count, this->_descriptor_set_layout.get());

        vk::DescriptorSetAllocateInfo alloc_info(
            this->_descriptor_pool.get(), layouts.size(), layouts.data());

        this->_descriptor_sets =
            this->_device.allocateDescriptorSetsUnique(alloc_info);
    }

    enum AttachmentType { INPUT, COLOR, RESOLVE, DEPTH_STENCIL };

    struct Attachment {
        vk::ImageLayout layout;
        vk::AttachmentDescription desc;
    };

    vk::UniqueRenderPass _create_render_pass() {
        auto &swapchain = this->_vk_ctx->get_swapchain();

        vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics);

        std::vector<vk::AttachmentDescription> attachments;

        std::vector<vk::AttachmentReference> color_refs;

        if (this->_enable_sample_buffer) {
            vk::AttachmentDescription sample_attachment(
                {}, swapchain.format, this->_msaa_samples,
                vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                vk::AttachmentLoadOp::eDontCare,
                vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
                vk::ImageLayout::eColorAttachmentOptimal);

            color_refs.push_back({static_cast<uint32_t>(attachments.size()),
                                  vk::ImageLayout::eColorAttachmentOptimal});
            attachments.push_back(sample_attachment);
        } else {
            vk::AttachmentDescription color_attachment(
                {}, swapchain.format, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                vk::AttachmentLoadOp::eDontCare,
                vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
                vk::ImageLayout::ePresentSrcKHR);

            color_refs.push_back({static_cast<uint32_t>(attachments.size()),
                                  vk::ImageLayout::eColorAttachmentOptimal});
            attachments.push_back(color_attachment);
        }

        vk::AttachmentReference depth_ref;
        if (this->_enable_depth_buffer) {
            vk::AttachmentDescription depth_attachment(
                {}, this->_vk_ctx->get_depth_format(), this->_msaa_samples,
                vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
                vk::AttachmentLoadOp::eDontCare,
                vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
                vk::ImageLayout::eDepthStencilAttachmentOptimal);

            depth_ref = {static_cast<uint32_t>(attachments.size()),
                         vk::ImageLayout::eDepthStencilAttachmentOptimal};
            attachments.push_back(depth_attachment);

            subpass.setPDepthStencilAttachment(&depth_ref);
        }

        if (this->_enable_sample_buffer) {
            vk::AttachmentDescription resolve_attachment(
                {}, swapchain.format, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
                vk::AttachmentLoadOp::eDontCare,
                vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
                vk::ImageLayout::ePresentSrcKHR);
            vk::AttachmentReference resolve_ref(
                {static_cast<uint32_t>(attachments.size()),
                 vk::ImageLayout::eColorAttachmentOptimal});
            attachments.push_back(resolve_attachment);

            subpass.setPResolveAttachments(&resolve_ref);
        } else {
            // vk::AttachmentDescription color_attachment(
            //     {}, swapchain.format, vk::SampleCountFlagBits::e1,
            //     vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
            //     vk::AttachmentLoadOp::eDontCare,
            //     vk::AttachmentStoreOp::eDontCare,
            //     vk::ImageLayout::eUndefined,
            //     vk::ImageLayout::ePresentSrcKHR);

            // color_refs.push_back({static_cast<uint32_t>(attachments.size()),
            //                       vk::ImageLayout::eColorAttachmentOptimal});
            // attachments.push_back(color_attachment);
        }

        subpass.setColorAttachmentCount(color_refs.size());
        subpass.setPColorAttachments(color_refs.data());

        vk::SubpassDependency dep(
            VK_SUBPASS_EXTERNAL, 0,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eColorAttachmentOutput, {},
            vk::AccessFlagBits::eColorAttachmentRead |
                vk::AccessFlagBits::eColorAttachmentWrite,
            {});

        vk::RenderPassCreateInfo create_info(
            {}, attachments.size(), attachments.data(), 1, &subpass, 1, &dep);

        auto render_pass = this->_device.createRenderPassUnique(create_info);

        this->_frame_buffers = this->_create_frame_buffers(render_pass.get());
        return render_pass;
    }

}; // namespace
} // namespace
namespace my {
std::shared_ptr<RenderDevice> make_vulkan_render_device(VulkanCtx *ctx) {
    return std::make_shared<VulkanRenderDevice>(ctx);
}

} // namespace my
