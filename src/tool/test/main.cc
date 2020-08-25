#include <media/audio_mgr.hpp>
#include <my_storage.hpp>

int main(int argc, char *argv[]) {
    auto async_task = my::AsyncTask::create();
    auto resource_mgr = my::ResourceMgr::create(async_task.get());

    auto audio = resource_mgr->load<my::Audio>("bgm/bgm_01.ogg").get();
    audio->play();
    sleep(5);
    Logger::get()->close();
    return 0;
}
