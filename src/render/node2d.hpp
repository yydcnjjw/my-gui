#pragma once

#include <core/core.hpp>
#include <window/window.hpp>

#include <render/type.hpp>

namespace my {
class Node {
    virtual ~Node() = default;
};
class Node2D : public std::enable_shared_from_this<Node2D> {
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

    Rect rect() { return Rect::Make(this->irect()); }

    void add_sub_node(shared_ptr<Node2D> n) {
        n->parent(this->shared_from_this());
        this->_sub_nodes.push_back(n);
    }

    std::list<shared_ptr<Node2D>> &sub_nodes() { return this->_sub_nodes; }

    template <typename Callback> void set_mouse_button_listener(Callback &&cb) {
        this->_mouse_button_listener = cb;
    }

    bool disptach_mouse_button_event(shared_ptr<Event<MouseButtonEvent>> e) {
        auto pos = (*e)->pos();
        auto rect = this->rect();

        if (this->parent() && !rect.intersect(this->parent()->rect())) {
            return false;
        }

        if (!rect.contains(pos.x(), pos.y())) {
            return false;
        }

        for (auto sub_node : this->sub_nodes()) {
            auto transalte_e = Event<MouseButtonEvent>::make(
                (*e)->make_translate(this->pos()));
            if (sub_node->disptach_mouse_button_event(transalte_e)) {
                return true;
            }
        }
        // on mouse button event
        return this->on_mouse_button(e);
    }

  private:
    shared_ptr<Node2D> _parent;
    std::list<shared_ptr<Node2D>> _sub_nodes;
    IPoint2D _pos;
    ISize2D _size;

    sk_sp<SkSurface> _surface;

    // event
    using mouse_button_listener_type =
        std::function<bool(shared_ptr<Event<MouseButtonEvent>> e)>;
    mouse_button_listener_type _mouse_button_listener;

    bool on_mouse_button(shared_ptr<Event<MouseButtonEvent>> e) {
        if (this->_mouse_button_listener) {
            return this->_mouse_button_listener(e);
        }
        return false;
    }

    IRect irect() {
        auto [x, y] = this->pos();
        auto [w, h] = this->size();
        return IRect::MakeXYWH(x, y, w, h);
    }
};

class RootNode2D : public Node2D, public Observer {
  public:
    template <typename... Args>
    RootNode2D(Args &&... args) : Node2D(std::forward<Args>(args)...) {}

    template <typename... Args>
    static shared_ptr<RootNode2D> make_root(Args &&... args) {
        return std::make_shared<RootNode2D>(std::forward<Args>(args)...);
    }
    void subscribe(Observable *o) override {
        on_event<MouseButtonEvent>(o).subscribe(
            [self = this->shared_from_this()](auto e) {
                self->disptach_mouse_button_event(e);
            });
    }

  private:
    using Node2D::make;
    using Node2D::parent;
};

} // namespace my
