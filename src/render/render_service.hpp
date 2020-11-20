#pragma once

#include <core/core.hpp>
#include <render/node2d.hpp>
#include <window/window.hpp>

namespace my {

class RenderService : public BasicService// , public Bindable
{
  public:
    RenderService() {}

    static unique_ptr<RenderService> make(shared_ptr<Window>);
    virtual SkCanvas *canvas() = 0;

    void connect(shared_ptr<Node2DTree> tree) { this->_node2dtree = tree; }
    void disconnect(shared_ptr<Node2DTree>) { this->_node2dtree.reset(); }

    template <typename Func> void run_at(Func &&func) {
        this->schedule<void>(
            [this, func = std::forward<Func>(func)]() { func(this); });
    }

protected:
    shared_ptr<Node2DTree> node2dtree() {
        return this->_node2dtree;
    }

  private:
    shared_ptr<Node2DTree> _node2dtree;
};

void Node2DTree::check_run_at_render_thread() {
    auto thread_info = this->renderer()->coordination().get_thread_info();
    if (!thread_info.at_this_thread()) {
        throw RenderServiceError("Node2D ui operations must render thread");
    }
}

} // namespace my
