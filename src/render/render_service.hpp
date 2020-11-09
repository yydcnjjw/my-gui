#pragma once

#include <core/core.hpp>
#include <window/window.hpp>
#include <render/node2d.hpp>

namespace my {

class RenderService : public BasicService {
  public:
    RenderService() {}

    static unique_ptr<RenderService> make(shared_ptr<Window>);
    virtual SkCanvas *canvas() = 0;

    void root_node(shared_ptr<RootNode2D> n) { this->_root_node = n; }

    shared_ptr<RootNode2D> root_node() { return this->_root_node; }

    template <typename Func> void run_at(Func &&func) {
        this->schedule<void>(
            [this, func = std::forward<Func>(func)]() { func(this); });
    }

    shared_ptr<RootNode2D> _root_node;
};

} // namespace my
