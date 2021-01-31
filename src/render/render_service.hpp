#pragma once

#include <core/core.hpp>
#include <render/node2d.hpp>
#include <window/window.hpp>

namespace my {

class RenderService;

class CanvasPtr {
public:
  CanvasPtr(SkCanvas *ptr, RenderService *renderer)
      : _ptr(ptr), _renderer(renderer) {}

  SkCanvas *operator->();

private:
  SkCanvas *_ptr;
  RenderService *_renderer;

  RenderService *renderer() { return this->_renderer; }
};

class RenderService : public BasicService // , public Bindable
{
public:
  RenderService() {}

  static unique_ptr<RenderService> make(shared_ptr<Window>);
  virtual CanvasPtr canvas() = 0;

  template <typename Func> void run_at(Func &&func) {
    this->schedule<void>(
        [this, func = std::forward<Func>(func)]() { func(this); });
  }

protected:
//   shared_ptr<NodeViewTree> node2dtree() { return this->_node2dtree; }

// private:
//   shared_ptr<NodeViewTree> _node2dtree;
};

SkCanvas *CanvasPtr::operator->() {
  auto thread_info = this->renderer()->coordination().get_thread_info();
  if (!thread_info.at_this_thread()) {
    throw RenderServiceError("Node2D ui operations must render thread");
  }
  return this->_ptr;
}

} // namespace my
