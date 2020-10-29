#pragma once
#include <core/config.hpp>
#include <my_render.hpp>
#include <tree.hh>
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

    const IPoint2D &pos() const { return this->_pos; }
    const ISize2D &size() const { return this->_size; }

    shared_ptr<Node2D> parent() const { return this->_parent; }
    void parent(shared_ptr<Node2D> parent) { this->_parent = parent; }

    IRect irect() {
        auto [x, y] = this->pos();
        auto [w, h] = this->size();
        return IRect::MakeXYWH(x, y, w, h);
    }

    Rect rect() {
        return Rect::Make(this->irect());
    }

    void add_sub_node(shared_ptr<Node2D> n) {
        n->parent(this->shared_from_this());
        this->_sub_node.push_back(n);
    }

    std::list<shared_ptr<Node2D>> &sub_nodes() { return this->_sub_node; }

  private:
    shared_ptr<Node2D> _parent;
    std::list<shared_ptr<Node2D>> _sub_node;
    IPoint2D _pos;
    ISize2D _size;

    sk_sp<SkSurface> _surface;
};

} // namespace my
