#include <render/canvas.h>
#define LLGL_ENABLE_UTILITY
#include <LLGL/Utility.h>

#include <util/codecvt.h>

#include <boost/format.hpp>
namespace my {

DrawPath::DrawPath(const glm::vec2 begin) { this->_points.push_back(begin); }

DrawPath &DrawPath::line_to(const glm::vec2 &pos) {
    this->_points.push_back(pos);
    return *this;
}
DrawPath &DrawPath::rect(const glm::vec2 &a, const glm::vec2 &c) {
    if (this->_points.size() == 1) {
        this->_points[0] = a;
    } else {
        this->line_to(a);
    }

    this->line_to({c.x, a.y});
    this->line_to(c);
    this->line_to({a.x, c.y});
    return *this;
}

Canvas::Canvas(RenderSystem *renderer, Window *win, ResourceMgr *resource_mgr,
               FontMgr *font_mgr)
    : _renderer(renderer) {

    auto win_size = win->get_size();
    {
        this->_vertex_format.AppendAttribute({"pos", LLGL::Format::RG32Float});
        this->_vertex_format.AppendAttribute({"uv", LLGL::Format::RG32Float});
        this->_vertex_format.AppendAttribute(
            {"color", LLGL::Format::RGBA8UNorm});
        this->_vertex_format.SetStride(sizeof(DrawVert));
    }

    {
        LLGL::RenderContextDescriptor context_desc;
        context_desc.videoMode.resolution = {win_size.w, win_size.h};
        context_desc.vsync.enabled = true;

        this->_context =
            renderer->CreateRenderContext(context_desc, win->get_surface());
    }

    {
        LLGL::ShaderProgram *shader_program = nullptr;
        auto vert =
            resource_mgr
                ->load_from_path<my::Blob>(
                    "/home/yydcnjjw/workspace/code/project/my-gui/assets/"
                    "shaders/canvas_shader.vert.spv")
                .get();
        auto frag =
            resource_mgr
                ->load_from_path<my::Blob>(
                    "/home/yydcnjjw/workspace/code/project/my-gui/assets/"
                    "shaders/canvas_shader.frag.spv")
                .get();

        LLGL::Shader *vert_shader = nullptr;
        LLGL::Shader *frag_shader = nullptr;

        LLGL::ShaderDescriptor vert_desc, frag_desc;
        vert_desc.type = LLGL::ShaderType::Vertex;
        vert_desc.source = (char *)vert->data();
        vert_desc.sourceSize = vert->size();
        vert_desc.sourceType = LLGL::ShaderSourceType::BinaryBuffer;

        vert_desc.vertex.inputAttribs = this->_vertex_format.attributes;

        frag_desc.type = LLGL::ShaderType::Fragment;
        frag_desc.source = (char *)frag->data();
        frag_desc.sourceSize = frag->size();
        frag_desc.sourceType = LLGL::ShaderSourceType::BinaryBuffer;

        vert_shader = renderer->CreateShader(vert_desc);
        frag_shader = renderer->CreateShader(frag_desc);

        for (auto shader : {vert_shader, frag_shader}) {
            if (shader != nullptr) {
                std::string log = shader->GetReport();
                if (!log.empty())
                    throw std::runtime_error(log);
            }
        }

        // Create shader program which is used as composite
        LLGL::ShaderProgramDescriptor shader_program_desc;
        {
            shader_program_desc.vertexShader = vert_shader;
            shader_program_desc.fragmentShader = frag_shader;
        }
        shader_program = renderer->CreateShaderProgram(shader_program_desc);

        // Link shader program and check for errors
        if (shader_program->HasErrors())
            throw std::runtime_error(shader_program->GetReport());

        {
            LLGL::PipelineLayoutDescriptor layout_desc{
                {LLGL::BindingDescriptor{LLGL::ResourceType::Buffer,
                                         LLGL::BindFlags::ConstantBuffer,
                                         LLGL::StageFlags::VertexStage, 0},
                 LLGL::BindingDescriptor{LLGL::ResourceType::Texture,
                                         LLGL::BindFlags::Sampled,
                                         LLGL::StageFlags::FragmentStage, 1},
                 LLGL::BindingDescriptor{LLGL::ResourceType::Sampler, 0,
                                         LLGL::StageFlags::FragmentStage, 2}}};

            this->_pipeline_layout =
                renderer->CreatePipelineLayout(layout_desc);
        }

        {
            this->_const_block.scale =
                glm::vec2(2.0f / win_size.w, 2.0f / win_size.h);
            this->_const_block.translate = glm::vec2(-1.0f);
            this->_constant = renderer->CreateBuffer(
                LLGL::ConstantBufferDesc(sizeof(ConstBlock)),
                &this->_const_block);

            this->_default_font = font_mgr->add_font(
                "/home/yydcnjjw/workspace/code/project/my-gui/assets/fonts/"
                "NotoSansCJK-Regular.ttc");
            int w, h;
            auto font_tex = this->_default_font->get_tex_as_rgb32(&w, &h);
            LLGL::SrcImageDescriptor src_image_desc{
                LLGL::ImageFormat::RGBA, LLGL::DataType::UInt8, font_tex,
                (uint32_t)w * h * 4};
            this->_font_tex = this->_renderer->CreateTexture(
                LLGL::Texture2DDesc(LLGL::Format::RGBA8UNorm, w, h),
                &src_image_desc);

            this->_sampler = this->_renderer->CreateSampler({});

            LLGL::ResourceHeapDescriptor resource_heap_desc;
            {
                resource_heap_desc.pipelineLayout = this->_pipeline_layout;
                resource_heap_desc.resourceViews = {
                    this->_constant, this->_font_tex, this->_sampler};
            }

            this->_default_resource =
                renderer->CreateResourceHeap(resource_heap_desc);
        }

        LLGL::GraphicsPipelineDescriptor pipeline_desc;
        {
            pipeline_desc.shaderProgram = shader_program;
            pipeline_desc.pipelineLayout = this->_pipeline_layout;
            pipeline_desc.rasterizer.cullMode = LLGL::CullMode::Back;
            pipeline_desc.blend.targets[0] = {true};
        }

        this->_pipeline = renderer->CreatePipelineState(pipeline_desc);
    }

    {
        this->_queue = renderer->GetCommandQueue();
        this->_commands = renderer->CreateCommandBuffer();
    }

    { this->_fence = renderer->CreateFence(); }
}

Canvas::~Canvas() {

    // this->_renderer->Release(*this->_font_tex);
    // this->_renderer->Release(*this->_default_resource);
    // this->_renderer->Release(*this->_pipeline_layout);
    // this->_renderer->Release(*this->_pipeline);
    // this->_renderer->Release(*this->_context);

    // TODO: release textures
}

Canvas &Canvas::draw_image(std::shared_ptr<Image> image, const glm::vec2 &p_min,
                           const glm::vec2 &p_max, const glm::vec2 &uv_min,
                           const glm::vec2 &uv_max, uint8_t alpha) {
    if (alpha == 0) {
        return *this;
    }

    this->_save();
    this->_get_state().image = image;

    {
        std::unique_lock<std::shared_mutex> l_lock(this->_lock);
        LLGL::SrcImageDescriptor src_image_desc{
            LLGL::ImageFormat::RGBA, LLGL::DataType::UInt8, image->raw_data(),
            image->width() * image->height() * 4};
        auto texture = this->_renderer->CreateTexture(
            LLGL::Texture2DDesc(LLGL::Format::RGBA8UNorm, image->width(),
                                image->height()),
            &src_image_desc);

        LLGL::ResourceHeapDescriptor resource_heap_desc;
        {
            resource_heap_desc.pipelineLayout = this->_pipeline_layout;
            resource_heap_desc.resourceViews = {this->_constant, texture,
                                                this->_sampler};
        }

        auto resource = this->_renderer->CreateResourceHeap(resource_heap_desc);
        this->_textures.insert({image->get_key(), {texture, resource}});
    }

    this->_prim_rect_uv(p_min, p_max, uv_min, uv_max, {255, 255, 255, alpha});

    this->_restore();
    return *this;
}

Canvas &Canvas::fill_text(const std::string &text, const glm::vec2 &p,
                          my::Font *font, float font_size,
                          const ColorRGBAub &color) {
    std::wstring wtext = codecvt::utf_to_utf<wchar_t>(text);
    this->_save();
    this->_get_state().image = nullptr;
    if (font == nullptr) {
        font = this->_default_font;
    }

    float scale = font_size / font->font_size();

    glm::vec2 pos = p;
    pos.y += font_size;
    glm::vec2 p_min;
    glm::vec2 p_max;
    for (auto ch : wtext) {
        const auto &glyph = font->get_glyph(ch);
        const auto w = glyph.w * scale;
        const auto h = glyph.h * scale;
        p_min.x = pos.x + glyph.bearing.x;
        p_min.y = pos.y - (h - glyph.h * scale) - h;
        p_max.x = p_min.x + w;
        p_max.y = p_min.y + h;
        this->stroke_rect(p_min, p_max, {255, 0, 0, 255}, 1);
        this->_prim_rect_uv(p_min, p_max, glyph.uv0, glyph.uv1, color);
        pos.x += glyph.advance_x * scale;
    }
    this->_restore();
    return *this;
}

const std::vector<DrawCmd> &Canvas::get_draw_cmd() {
    this->_add_cmd();
    std::shared_lock<std::shared_mutex> l_lock(this->_lock);
    return this->_cmd_list;
}

void Canvas::_add_cmd() {
    std::unique_lock<std::shared_mutex> l_lock(this->_lock);
    auto idx_count = this->_idx_list.size();
    uint32_t elem_count;
    if (this->_cmd_list.empty()) {
        elem_count = idx_count;
    } else {
        auto &prev_cmd = this->_cmd_list.back();
        auto prev_idx_offset = prev_cmd.idx_offset;
        auto prev_elem_count = prev_cmd.elem_count;
        elem_count = idx_count - (prev_idx_offset + prev_elem_count);
    }

    if (elem_count == 0) {
        return;
    }

    this->_current_cmd.elem_count = elem_count;
    this->_cmd_list.push_back(this->_current_cmd);
    this->_current_cmd.idx_offset = idx_count;
    this->_current_cmd.vtx_offset = this->_vtx_list.size();
}

void Canvas::_save() {
    {
        auto &current_state = this->_get_state();
        std::unique_lock<std::shared_mutex> l_lock(this->_lock);
        this->_state_stack.push(current_state);
    }
    this->_add_cmd();
}
void Canvas::_restore() {
    this->_add_cmd();

    std::unique_lock<std::shared_mutex> l_lock(this->_lock);
    auto &state = this->_state_stack.top();
    this->_state_stack.pop();
    l_lock.unlock();

    this->_set_state(state);
}

void Canvas::_prim_rect(const glm::vec2 &a, const glm::vec2 &c,
                        const ColorRGBAub &col) {
    this->_prim_rect_uv(a, c, {0, 0}, {0, 0}, col);
}

void Canvas::_prim_rect_uv(const glm::vec2 &a, const glm::vec2 &c,
                           const glm::vec2 &uv_a, const glm::vec2 &uv_c,
                           const ColorRGBAub &col) {
    std::unique_lock<std::shared_mutex> l_lock(this->_lock);
    glm::vec2 b(c.x, a.y), d(a.x, c.y), uv_b(uv_c.x, uv_a.y),
        uv_d(uv_a.x, uv_c.y);
    auto idx = this->_vtx_list.size();
    this->_vtx_list.push_back({a, uv_a, col});
    this->_vtx_list.push_back({b, uv_b, col});
    this->_vtx_list.push_back({c, uv_c, col});
    this->_vtx_list.push_back({d, uv_d, col});
    this->_idx_list.push_back(idx);
    this->_idx_list.push_back(idx + 1);
    this->_idx_list.push_back(idx + 2);
    this->_idx_list.push_back(idx);
    this->_idx_list.push_back(idx + 2);
    this->_idx_list.push_back(idx + 3);
}

void Canvas::_add_poly_line(const DrawPath &path, const ColorRGBAub &col,
                            float line_width) {
    std::unique_lock<std::shared_mutex> l_lock(this->_lock);
    const size_t point_count = path._points.size();

    if (point_count < 2) {
        return;
    }

    glm::vec2 uv = this->_default_font->white_pixels_uv();

    size_t count = point_count - 1;

    for (size_t i = 0; i < count; ++i) {
        const auto i2 = i + 1;
        assert(i2 < point_count);
        size_t current_idx = this->_vtx_list.size();

        const glm::vec2 &p1 = path._points[i];
        const glm::vec2 &p2 = path._points[i2];

        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;

        float d2 = dx * dx + dy * dy;
        if (d2 > 0.0f) {
            float inv_len = 1.0f / std::sqrt(d2);
            dx *= inv_len;
            dy *= inv_len;
        }

        dx *= (line_width * 0.5f);
        dy *= (line_width * 0.5f);

        this->_vtx_list.push_back({{p1.x + dy, p1.y - dx}, uv, col});
        this->_vtx_list.push_back({{p2.x + dy, p2.y - dx}, uv, col});
        this->_vtx_list.push_back({{p2.x - dy, p2.y + dx}, uv, col});
        this->_vtx_list.push_back({{p1.x - dy, p1.y + dx}, uv, col});

        this->_idx_list.push_back(current_idx);
        this->_idx_list.push_back(current_idx + 1);
        this->_idx_list.push_back(current_idx + 2);
        this->_idx_list.push_back(current_idx);
        this->_idx_list.push_back(current_idx + 2);
        this->_idx_list.push_back(current_idx + 3);
    }
}

void Canvas::_add_convex_poly_fill(const DrawPath &path,
                                   const ColorRGBAub &col) {
    std::unique_lock<std::shared_mutex> l_lock(this->_lock);
    const size_t point_count = path._points.size();

    glm::vec2 uv = this->_default_font->white_pixels_uv();

    // const size_t idx_count = (point_count - 2) * 3;
    const size_t vtx_count = point_count;

    size_t current_idx = this->_vtx_list.size();

    for (size_t i = 0; i < vtx_count; ++i) {
        this->_vtx_list.push_back({path._points[i], uv, col});
    }

    for (size_t i = 2; i < point_count; i++) {
        this->_idx_list.push_back(current_idx);
        this->_idx_list.push_back(current_idx + i - 1);
        this->_idx_list.push_back(current_idx + i);
    }
}

void Canvas::render() {
    auto &draw_cmd = this->get_draw_cmd();
    {
        std::unique_lock<std::shared_mutex> l_lock(this->_lock);

        if (this->_vtx_list.empty() && this->_idx_list.empty()) {
            return;
        }

        this->_queue->WaitIdle();
        if (this->_vtx_buf) {
            this->_renderer->Release(*this->_vtx_buf);
            this->_vtx_buf = nullptr;
        }
        if (this->_idx_buf) {
            this->_renderer->Release(*this->_idx_buf);
            this->_idx_buf = nullptr;
        }

        this->_vtx_buf = this->_renderer->CreateBuffer(
            LLGL::VertexBufferDesc(this->_vtx_list.size() * sizeof(DrawVert),
                                   this->_vertex_format),
            this->_vtx_list.data());
        this->_idx_buf = this->_renderer->CreateBuffer(
            LLGL::IndexBufferDesc(this->_idx_list.size() * sizeof(uint32_t),
                                  LLGL ::Format::R32UInt),
            this->_idx_list.data());

        this->_commands->Begin();
        {
            // Set viewport and scissor rectangle
            this->_commands->SetViewport(this->_context->GetResolution());

            this->_commands->SetPipelineState(*this->_pipeline);

            // Set vertex buffer
            this->_commands->SetVertexBuffer(*this->_vtx_buf);
            this->_commands->SetIndexBuffer(*this->_idx_buf);

            this->_commands->BeginRenderPass(*this->_context);
            {
                // this->_commands->Clear(LLGL::ClearFlags::Color);
                for (const auto &cmd : draw_cmd) {
                    if (cmd.state.image) {
                        auto texture =
                            this->_textures.at(cmd.state.image->get_key())
                                .resource;

                        this->_commands->SetResourceHeap(*texture);
                    } else {
                        this->_commands->SetResourceHeap(
                            *this->_default_resource);
                    }

                    this->_commands->DrawIndexed(cmd.elem_count, 0,
                                                 cmd.vtx_offset);
                }
            }
            this->_commands->EndRenderPass();
        }
        this->_commands->End();

        this->_queue->Submit(*this->_commands);

        this->_context->Present();
    }
    this->clear();
}

} // namespace my
