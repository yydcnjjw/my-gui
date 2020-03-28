#pragma once

#include <aio.h>

namespace my {

class Archive {
  public:
    Archive() = default;
    virtual ~Archive() = default;

    static std::shared_ptr<Archive> make_xp3(const fs::path &);
    
    class Stream {
    public:
        Stream() = default;
        virtual ~Stream() = default;
        virtual std::string read_all() = 0;
    };
    
    virtual std::shared_ptr<Stream> open(const my::fs::path &path) = 0;
};

} // namespace my
