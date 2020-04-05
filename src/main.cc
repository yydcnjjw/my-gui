// #include "camera.hpp"
// #include "canvas.h"
// #include "draw_list.h"
// #include "font_mgr.h"
// #include "logger.h"
// #include "render_device.h"
// #include "resource_mgr.h"
// #include "util.hpp"
// #include "vulkan_ctx.h"
// #include "window_mgr.h"

// #include <SDL2/SDL.h>
// #define GLM_FORCE_RADIANS
// #define GLM_FORCE_DEPTH_ZERO_TO_ONE
// #define GLM_ENABLE_EXPERIMENTAL
// #include <glm/glm.hpp>
// #include <glm/gtc/matrix_transform.hpp>
// #include <glm/gtx/hash.hpp>
// #define TINYOBJLOADER_IMPLEMENTATION
// #include "tiny_obj_loader.h"

// #include <thread>

// const std::string MODEL_PATH = "../assets/models/chalet.obj";
// const std::string TEXTURE_PATH = "../assets/textures/chalet.jpg";
// const std::string TEXTURE2_PATH = "../assets/textures/texture.jpg";

// struct UniformBufferObject {
//     glm::mat4 view;
//     glm::mat4 proj;
// };

// struct UniformBufferPerObject {
//     glm::mat4 model;
// };

// struct PushConstBlock {
//     float roughness;
//     float metallic;
//     float r, g, b;
// };

// void load_model(std::vector<my::Vertex> &vertices,
//                 std::vector<uint32_t> &indices) {
//     tinyobj::attrib_t attrib;
//     std::vector<tinyobj::shape_t> shapes;
//     std::vector<tinyobj::material_t> materials;
//     std::string warn, err;

//     if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
//                           MODEL_PATH.c_str())) {
//         throw std::runtime_error(warn + err);
//     }

//     for (const auto &shape : shapes) {
//         for (const auto &index : shape.mesh.indices) {
//             my::Vertex vertex = {};

//             vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
//                           attrib.vertices[3 * index.vertex_index + 1],
//                           attrib.vertices[3 * index.vertex_index + 2]};

//             vertex.tex_coord = {
//                 attrib.texcoords[2 * index.texcoord_index + 0],
//                 1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

//             // if (unique_vertices.count(vertex) == 0) {
//             // unique_vertices[vertex] =
//             // static_cast<uint32_t>(vertices.size());
//             vertices.push_back(vertex);
//             // }

//             indices.push_back(indices.size());
//         }
//     }
// }
// #include "application.h"

// void run() {
// try {
// auto win_mgr = my::get_window_mgr();
// auto win = win_mgr->create_window("Test", 800, 600);
//     auto resource_mgr = my::get_resource_mgr();

//     bool is_quit = false;

//     my::Camera camera(win_mgr);

//     auto tex1 =
//         resource_mgr->load_from_path<my::Texture>(TEXTURE_PATH).get();
//     auto tex2 =
//         resource_mgr->load_from_path<my::Texture>(TEXTURE_PATH).get();

//     std::vector<my::Vertex> vertices;
//     std::vector<uint32_t> indices;
//     load_model(vertices, indices);

//     auto draw_thread = std::thread([&]() {
//         auto ctx = my::make_vulkan_ctx(win);
//         auto device = my::make_vulkan_render_device(ctx.get());
//         auto canvas = my::make_vulkan_canvas(ctx.get(), device.get());

//         auto vertex_binding = device->create_vertex_buffer(vertices);

//         auto index_binding = device->create_index_buffer(indices);

//         auto texture1_buffer = device->create_texture_buffer(
//             tex1->pixels, tex1->width, tex1->height, true);
//         auto texture2_buffer = device->create_texture_buffer(
//             tex2->pixels, tex2->width, tex2->height, true);

//         auto uniform_buffer = device->create_uniform_buffer(
//             sizeof(UniformBufferObject), sizeof(UniformBufferPerObject),
//             2);

//         auto vert_shader = device->create_shader(
//             my::aio::file_read_all("../assets/shaders/shader.vert.spv")
//                 .get(),
//             vk::ShaderStageFlagBits ::eVertex, "main");
//         auto frag_shader = device->create_shader(
//             my::aio::file_read_all("../assets/shaders/shader.frag.spv")
//                 .get(),
//             vk::ShaderStageFlagBits ::eFragment, "main");

//         my::VertexDesciption vertex_desc = {
//             {my::Vertex::get_binding_description()},
//             my::Vertex::get_attribute_descriptions()};

//         auto pipeline = device->create_pipeline({vert_shader,
//         frag_shader},
//                                                 vertex_desc, true);
//         auto &swapchain = ctx->get_swapchain();
//         GLOG_I("draw begin ...");

//         std::vector<UniformBufferPerObject> dubos{
//             {glm::mat4(1.0f)},
//             {glm::translate(glm::mat4(1.0f), {1.0f, 2.0f, 1.0f})}};
//         device->map_memory(
//             *uniform_buffer.dynamic_binding, uniform_buffer.dynamic_size,
//             [&](void *dst) {
//                 for (const auto &ubo : dubos) {
//                     memcpy(dst, &ubo.model, sizeof(ubo.model));
//                     dst = (uint8_t *)dst + uniform_buffer.dynamic_align;
//                 }
//             });

//         rxcpp::observable<>::interval(std::chrono::milliseconds(1000 /
//         30))
//             .take_while([&](auto) { return !is_quit; })
//             .subscribe(
//                 [&](auto) {
//                     UniformBufferObject ubo = {};
//                     ubo.view = camera.view;

//                     ubo.proj =
//                         glm::perspective(glm::radians(45.0f),
//                                          swapchain.extent.width /
//                                              (float)swapchain.extent.height,
//                                          0.1f, 10.0f);
//                     device->copy_to_buffer(&ubo, sizeof(ubo),
//                                            *uniform_buffer.share_binding);

//                     ctx->prepare_buffer();

//                     device->draw_begin();

//                     device->bind_pipeline(pipeline.get());
//                     device->bind_vertex_buffer(vertex_binding);
//                     device->bind_index_buffer(index_binding);
//                     device->bind_texture_buffer(texture1_buffer,
//                     *pipeline);

//                     device->bind_uniform_buffer(uniform_buffer, 0,
//                                                 *pipeline);
//                     device->draw(indices.size());

//                     device->bind_uniform_buffer(uniform_buffer, 1,
//                                                 *pipeline);
//                     device->draw(indices.size());

//                     canvas->fill_text("Hello World!", {0, 0});
//                     canvas->draw();

//                     device->draw_end();

//                     ctx->swap_buffer();
//                 },
//                 [&]() { device->wait_idle(); });
//         GLOG_I("draw end ...");
//     });

//     win_mgr->event(my::EventType::EVENT_QUIT)
//         .as_blocking()
//         .subscribe([&is_quit](const std::shared_ptr<my::Event> &) {
//             is_quit = true;
//         });

//     if (draw_thread.joinable()) {
//         draw_thread.join();
//     }

//     GLOG_I("application end!");
// } catch (std::exception &e) {
//     GLOG_E(e.what());
// }
// }

#include <fstream>
#include <iostream>

#include <boost/locale/encoding.hpp>

// #include <codecvt.h>
// #include <font_mgr.h>
// #include <storage/archive.h>
#include <async_task.hpp>

namespace conv = boost::locale::conv;
int main(int, char *[]) {
    // try {
    //     auto xp3_archive =
    //     my::Archive::make_xp3("../assets/models/data.xp3"); auto iss =
    //     xp3_archive->open("system/initialize.tjs"); auto s = iss->read_all();
    //     std::ofstream ofs("initialize.tjs");
    //     ofs << s;
    // } catch (std::exception &e) {
    //     std::cout << e.what() << std::endl;
    // }

    // auto font_mgr = my::FontMgr::create();

    // auto font =
    // font_mgr->add_font("../assets/fonts/NotoSansCJK-Regular.ttc");
    // font->get_tex_as_alpha();
    // auto app = my::new_application(argc, argv);
    // app->run();
    // run();
    auto async_task = my::AsyncTask::create();
    auto timer = async_task->create_timer_interval(
        std::function<void(void)>([]() -> void {
            std::cout
                << std::chrono::steady_clock::now().time_since_epoch().count()
                << std::endl;
        }),
        std::chrono::milliseconds(1000));

    sleep(2);

    timer->start();

    sleep(5);

    timer->cancel();

    sleep(2);

    timer->start();

    sleep(5);

    timer->cancel();
    return 0;
}
