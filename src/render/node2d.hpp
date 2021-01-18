#pragma once

#include <core/core.hpp>
#include <window/window.hpp>

#include <render/exception.hpp>
#include <render/type.hpp>
// #include <render/render_service.hpp>

namespace my {
class Node2DTree;
class RenderService;

class Node {
  virtual ~Node() = default;
};

class Node2D : public std::enable_shared_from_this<Node2D>, public Observable {
public:
  Node2D(IRect const &rect)
      : _pos(rect.topLeft()), _size(rect.size()),
        _surface(SkSurface::MakeRasterN32Premul(this->_size.width(),
                                                this->_size.height())) {}
  Node2D(ISize2D const &size) : Node2D(IRect::MakeSize(size)) {}

  template <typename... Args> static shared_ptr<Node2D> make(Args &&...args) {
    return std::make_shared<Node2D>(std::forward<Args>(args)...);
  }

  void connect(shared_ptr<Node2DTree> tree) { this->_tree = tree; }

  void disconnect(shared_ptr<Node2DTree>) { this->_tree.reset(); }

  shared_ptr<Node2D> const &parent() const { return this->_parent; }

  shared_ptr<Node2D> parent() { return this->_parent; }

  void parent(shared_ptr<Node2D> parent);

  void add_sub_node(shared_ptr<Node2D> n);

  std::list<shared_ptr<Node2D>> const &sub_nodes() const {
    return this->_sub_nodes;
  }

  // prop
  IPoint2D const &pos() const { return this->_pos; }
  ISize2D const &size() const { return this->_size; }
  Rect rect() const { return Rect::Make(this->irect()); }

  void focusable(bool v) {
    if (this->_focusable == v) {
      return;
    }
    this->_focusable = v;
    if (v) {
      // TODO: join focus chain
    } else {
      // TODO: remove focus chain
    }
  }

  void focused(bool v) {
    if (this->_focused == v) {
      return;
    }
    this->_focused = v;
    if (v) {
      // TODO:
    } else {
    }
  }

  // render
  SkCanvas &canvas();
  sk_sp<SkImage> snapshot();

  // event
  bool disptach_mouse_button_event(event_type<MouseButtonEvent> e) {
    auto pos = (*e)->pos();
    auto rect = this->rect();

    if (this->parent() && !rect.intersect(this->parent()->rect())) {
      return false;
    }

    if (!rect.contains(pos.x(), pos.y())) {
      return false;
    }

    for (auto sub_node : this->sub_nodes()) {
      auto transalte_e =
          Event<MouseButtonEvent>::make((*e)->make_translate(this->pos()));
      if (sub_node->disptach_mouse_button_event(transalte_e)) {
        return true;
      }
    }
    return this->post(e);
  }

  observable_type event_source() override {
    
  }

  template <typename EventDataType, typename Callback>
  void bind_event(Callback &&callback) {
    auto &type_id = typeid(EventDataType);
    if (this->is_binded<EventDataType>()) {
      GLOG_W("%s event has been binded and will be replaced by default",
             type_id.name());
    }
    this->_binded_events.emplace(type_id,
                                 callback(on_event<EventDataType>(this)));
  }
  template <typename EventDataType> void unbind_event() {
    if (this->is_binded<EventDataType>()) {
      this->_binded_events.erase(typeid(EventDataType));
    }
  }

private:
  shared_ptr<Node2DTree> _tree;
  shared_ptr<Node2D> _parent;
  std::list<shared_ptr<Node2D>> _sub_nodes;

  // prop
  IPoint2D _pos;
  ISize2D _size;

  bool _focusable{false};
  bool _focused{false};

  // render
  sk_sp<SkSurface> _surface;

  // event
  std::map<std::type_index, rx::composite_subscription> _binded_events;

  IRect irect() const {
    auto [x, y] = this->pos();
    auto [w, h] = this->size();
    return IRect::MakeXYWH(x, y, w, h);
  }

  shared_ptr<Node2DTree> tree() {
    if (!this->_tree) {
      throw RenderServiceError("Not connected Node2D tree");
    }
    return this->_tree;
  }

  template <typename EventDataType> bool is_binded() {
    return this->_binded_events.count(typeid(EventDataType));
  }

  template <typename EventDataType> bool post(event_type<EventDataType> e) {
    if (this->is_binded<EventDataType>()) {
      // this->post(e->as_dynamic());
      return true;
    } else {
      return false;
    }
  }
};

class Node2DTree : public Observer,
                   public std::enable_shared_from_this<Node2DTree> {
public:
  static shared_ptr<Node2DTree> make() {
    return std::make_shared<Node2DTree>();
  }

  void subscribe(Observable *o) override {
    on_event<MouseButtonEvent>(o).subscribe(
        [self = this->shared_from_this()](auto e) {
          // self->disptach_mouse_button_event(e);
        });
  }

  void connect(RenderService *renderer) { this->_renderer = renderer; }

  void disconnect(RenderService *) { this->_renderer = nullptr; }

  void connect(shared_ptr<Node2D> root) { this->_root = root; }

  void disconnect(shared_ptr<Node2D>) { this->_root = nullptr; }

  inline void check_run_at_render_thread();

  shared_ptr<Node2D> root() {
    if (!this->_renderer) {
      throw RenderServiceError("Not connected Node2D root");
    }
    return this->_root;
  }

private:
  RenderService *_renderer;
  shared_ptr<Node2D> _root;

  RenderService *renderer() {
    if (!this->_renderer) {
      throw RenderServiceError("Not connected renderer");
    }
    return this->_renderer;
  }
};

void Node2D::parent(shared_ptr<Node2D> parent) {
  // this->tree()->check_run_at_render_thread();

  this->connect(parent->tree());
  this->_parent = parent;
}

void Node2D::add_sub_node(shared_ptr<Node2D> n) {
  // this->tree()->check_run_at_render_thread();
  this->_sub_nodes.push_back(n);
  n->parent(this->shared_from_this());
}

SkCanvas &Node2D::canvas() {
  // this->tree()->check_run_at_render_thread();
  return *this->_surface->getCanvas();
}

sk_sp<SkImage> Node2D::snapshot() {
  // this->tree()->check_run_at_render_thread();
  return this->_surface->makeImageSnapshot();
}

} // namespace my
