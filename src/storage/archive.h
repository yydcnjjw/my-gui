#pragma once

#include <application.h>

namespace my {

class Archive {
  public:
    Archive() = default;
    virtual ~Archive() = default;

    static std::shared_ptr<Archive> make_xp3(const fs::path &);
    virtual bool exists(const fs::path &) = 0;
    
    class Stream {
    public:
        Stream() = default;
        virtual ~Stream() = default;
        virtual std::string read_all() = 0;
    };

    virtual std::shared_ptr<Stream> open(const fs::path &path) = 0;
};

} // namespace my
