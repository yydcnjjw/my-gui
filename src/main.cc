#include "logger.h"
#include "util.hpp"

#include "camera.hpp"
#include "canvas.h"
#include "draw_list.h"
#include "render_device.h"
#include "vulkan_ctx.h"
#include "window_mgr.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <SDL2/SDL.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <thread>

const std::string MODEL_PATH = "models/chalet.obj";
const std::string TEXTURE_PATH = "textures/chalet.jpg";
// const std::string TEXTURE_PATH = "textures/texture.jpg";

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

void load_model(std::vector<my::Vertex> &vertices,
                std::vector<uint32_t> &indices) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                          MODEL_PATH.c_str())) {
        throw std::runtime_error(warn + err);
    }

    for (const auto &shape : shapes) {
        for (const auto &index : shape.mesh.indices) {
            my::Vertex vertex = {};

            vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
                          attrib.vertices[3 * index.vertex_index + 1],
                          attrib.vertices[3 * index.vertex_index + 2]};

            vertex.tex_coord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

            // if (unique_vertices.count(vertex) == 0) {
            // unique_vertices[vertex] =
            // static_cast<uint32_t>(vertices.size());
            vertices.push_back(vertex);
            // }

            indices.push_back(indices.size());
        }
    }
}

int main(int argc, char *argv[]) {
    try {
        auto win_mgr = my::get_window_mgr();
        auto win = win_mgr->create_window("Test", 800, 600);

        bool is_quit = false;

        my::Camera camera(win_mgr);

        int tex_width, tex_height, tex_channels;
        stbi_uc *pixels = stbi_load(TEXTURE_PATH.c_str(), &tex_width,
                                    &tex_height, &tex_channels,
                                    STBI_rgb_alpha);
        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        std::vector<my::Vertex> vertices;
        std::vector<uint32_t> indices;
        load_model(vertices, indices);

        auto draw_thread = std::thread([&]() {
            auto ctx = my::make_vulkan_ctx(win);
            auto device = my::make_vulkan_render_device(ctx.get());
            auto canvas = my::make_vulkan_canvas(device.get());

            auto vertex_binding = device->create_vertex_buffer(vertices);

            auto index_binding = device->create_index_buffer(indices);

            auto uniform_buffer =
                device->create_uniform_buffer(sizeof(UniformBufferObject));

            auto texture_buffer =
                device->create_texture_buffer(pixels, tex_width, tex_height);

            auto vert_shader = device->create_shader(
                Util::read_file("shaders/shader.vert.spv"),
                vk::ShaderStageFlagBits ::eVertex, "main");
            auto frag_shader = device->create_shader(
                Util::read_file("shaders/shader.frag.spv"),
                vk::ShaderStageFlagBits ::eFragment, "main");

            my::VertexDesciption vertex_desc = {
                {my::Vertex::get_binding_description()},
                my::Vertex::get_attribute_descriptions()};

            auto pipeline = device->create_pipeline(
                {vert_shader, frag_shader}, vertex_desc);

            auto &swapchain = ctx->get_swapchain();
            GLOG_I("draw begin ...");

            while (!is_quit) {
                UniformBufferObject ubo = {};
                ubo.model = glm::mat4(1.0f);
                ubo.view = camera.view;
                // ubo.view = glm::lookAt(glm::vec3(5.0f, 5.0f, .0f),
                //                        glm::vec3(0.0f, 0.0f, 0.0f),
                //                        glm::vec3(0.0f, 0.0f, 1.0f));
                ubo.proj = glm::perspective(glm::radians(45.0f),
                                            swapchain.extent.width /
                                                (float)swapchain.extent.height,
                                            0.1f, 10.0f);
                // ubo.proj[1][1] *= -1;

                device->copy_to_buffer(&ubo, sizeof(ubo),
                                       *uniform_buffer.binding);
                ctx->prepare_buffer();

                device->draw_begin();

                canvas->stroke_rect({0.0f, 0.0f}, {0.5f, 0.5f}, {0, 0, 255, 255}, 0.01f);
                // canvas->fill_rect({0.5f, 0.5f}, {1.0f, 1.0f}, {0, 255, 0, 255});
                // canvas->fill_rect({-0.5f, -0.5f}, {0.5f, 0.5f},
                //                   {255, 0, 0, 255});
                canvas->draw();
                
                device->bind_pipeline(pipeline.get());
                device->bind_vertex_buffer(vertex_binding);
                device->bind_index_buffer(index_binding);

                device->draw(indices.size());

                device->draw_end();

                ctx->swap_buffer();
            }

            device->wait_idle();
            GLOG_I("draw end ...");
        });

        win_mgr->event(my::EventType::EVENT_QUIT)
            .as_blocking()
            .subscribe([&is_quit](const std::shared_ptr<my::Event> &e) {
                GLOG_I("application quit!");
                is_quit = true;
            });

        if (draw_thread.joinable()) {
            draw_thread.join();
        }
        GLOG_I("application end!");
        stbi_image_free(pixels);

    } catch (std::exception &e) {
        GLOG_E(e.what());
    }
    return 0;
}
