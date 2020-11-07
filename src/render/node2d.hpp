#pragma once
#include <core/config.hpp>
#include <core/event_bus.hpp>
#include <window/event.hpp>

#include <my_render.hpp>

namespace my {
class Node {
    virtual ~Node() = default;
};
class Node2D : public std::enable_shared_from_this<Node2D>, public Observer {
  public:
    Node2D(IRect const &rect)
        : _pos(rect.topLeft()), _size(rect.size()),
          _surface(SkSurface::MakeRasterN32Premul(this->_size.width(),
                                                  this->_size.height())) {}
    Node2D(ISize2D const &size) : Node2D(IRect::MakeSize(size)) {}

    template <typename... Args>
    static shared_ptr<Node2D> make(Args &&... args) {
        return std::make_shared<Node2D>(std::forward<Args>(args)...);
    }

    SkCanvas *canvas() { return this->_surface->getCanvas(); }
    sk_sp<SkImage> snapshot() { return this->_surface->makeImageSnapshot(); }

    IPoint2D const &pos() const { return this->_pos; }
    ISize2D const &size() const { return this->_size; }

    shared_ptr<Node2D> parent() const { return this->_parent; }
    void parent(shared_ptr<Node2D> parent) { this->_parent = parent; }

    Rect region() { return Rect::Make(this->i_region()); }

    void add_sub_node(shared_ptr<Node2D> n) {
        n->parent(this->shared_from_this());
        this->_sub_nodes.push_back(n);
    }

    std::list<shared_ptr<Node2D>> &sub_nodes() { return this->_sub_nodes; }

    bool disptach_mouse_button_event(shared_ptr<Event<MouseButtonEvent>> e) {
        auto pos = e->pos;

        
        for (auto sub_node : this->sub_nodes()) {
            
            if (sub_node->disptach_mouse_button_event(e)) {
                return true;
            }

            // on mouse button event
        }
    }

    void subscribe(Observable *o) override {
        this->_event_source = o->event_source();
    }

  private:
    shared_ptr<Node2D> _parent;
    std::list<shared_ptr<Node2D>> _sub_nodes;
    IPoint2D _pos;
    ISize2D _size;

    sk_sp<SkSurface> _surface;

    // event
    Observable::observable_type _event_source;

    IRect i_region() {
        auto [x, y] = this->pos();
        auto [w, h] = this->size();
        return IRect::MakeXYWH(x, y, w, h);
    }    
};

} // namespace my
