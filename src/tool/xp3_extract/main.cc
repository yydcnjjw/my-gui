#include <storage/archive.h>
#include <storage/resource.hpp>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        return -1;
    }

    my::fs::path path{argv[1]};

    auto xp3_make = my::Archive::supported_archives().at(".xp3");

    auto xp3_archive = xp3_make(path);

    auto xp3_ofs = my::make_ofstream("xp3_files.txt");
    std::cout << "xp3 archive file counts: " << xp3_archive->list_files().size()
              << std::endl;
    for (auto name : xp3_archive->list_files()) {
        auto file_name = name;
        *xp3_ofs << file_name << std::endl;

        std::replace(name.begin(), name.end(), '\\', '/');

        auto path = "data" / my::fs::path(name);

        if (!my::fs::exists(path.parent_path())) {
            my::fs::create_directory(path.parent_path());
        }

        if (my::fs::is_directory(path)) {
            my::fs::create_directory(path);
        } else {
            auto data = xp3_archive->extract(file_name);
            auto ofs = my::make_ofstream(path);
            *ofs << data.is->rdbuf();
        }
    }
    Logger::get()->close();
    return 0;
}
