#include <storage/archive.h>
#include <storage/resource.hpp>

int main(int argc, char *argv[]) {

    my::program_options::options_description desc("xp3 extract options: ");

    desc.add_options()("help,h", "help")(
        "src,s", my::program_options::value<my::fs::path>(), "src file")(
        "dst,d", my::program_options::value<my::fs::path>(), "dst dir");

    my::program_options::variables_map vm;
    my::program_options::store(
        my::program_options::parse_command_line(argc, argv, desc), vm);
    my::program_options::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    if (!vm.count("src")) {
        return -1;
    }

    auto src_path = vm["src"].as<my::fs::path>();

    my::fs::path extract_path;
    if (vm.count("dst")) {
        extract_path = vm["dst"].as<my::fs::path>() / src_path.stem();
    } else {
        extract_path = src_path.parent_path() / src_path.stem();
    }

    if (!my::fs::exists(extract_path)) {
        my::fs::create_directory(extract_path);
    }

    auto xp3_make = my::Archive::supported_archives().at(".xp3");

    auto xp3_archive = xp3_make(src_path);

    auto xp3_ofs = my::make_ofstream("xp3_files.txt");
    GLOG_D("xp3 archive file counts: %d", xp3_archive->list_files().size());
    for (auto &file_name : xp3_archive->list_files()) {
        auto path = extract_path / my::fs::path(file_name);
        *xp3_ofs << path << std::endl;

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
