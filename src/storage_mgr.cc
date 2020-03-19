#include "util.hpp"
#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#include <boost/noncopyable.hpp>
#include <boost/thread/future.hpp>

#include <unordered_map>

typedef ID::UUID RID;
namespace my {

    class Resource : boost::noncopyable {};
typedef boost::future<Resource *> AsyncResource;
class ResourceMgr {
  public:
    ResourceMgr() = default;
    virtual ~ResourceMgr() = default;

    virtual AsyncResource load(const std::string &path) = 0;
    virtual void release() = 0;
};

} // namespace my
namespace {
class MyResourceMgr : public my::ResourceMgr {
  public:
    ~MyResourceMgr() override { my::AsyncResource r; };

  private:
    std::unordered_map<std::string, my::Resource *> resource_map;
};
} // namespace
