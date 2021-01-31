#pragma once

#include <core/core.hpp>
#include <window/window.hpp>

#include <render/exception.hpp>
#include <render/node.hpp>
#include <render/type.hpp>
// #include <render/render_service.hpp>

namespace my {
class RenderService;

class NodeView : public std::enable_shared_from_this<NodeView>, public Node {

  using ptr_type = shared_ptr<NodeView>;

public:
  static ptr_type make() { return shared_ptr<NodeView>(new NodeView()); }

  IPoint2D pos() const { return this->irect().topLeft(); }

  ISize2D size() const { return this->irect().size(); }

  int32_t x() const { return this->left(); }

  int32_t y() const { return this->top(); }

  int32_t top() const { return this->irect().top(); }

  int32_t left() const { return this->irect().left(); }

  IRect const &irect() const { return this->_irect; }

private:
  NodeView() : Node{} {}

  IRect &irect() { return this->_irect; }

  IRect _irect;
};

// class NodeView : public std::enable_shared_from_this<NodeView> {
// public:
//   NodeView(IRect const &rect)
//       : _pos(rect.topLeft()), _size(rect.size()),
//         _surface(SkSurface::MakeRasterN32Premul(this->_size.width(),
//                                                 this->_size.height())) {}
//   NodeView(ISize2D const &size) : NodeView(IRect::MakeSize(size)) {}

//   template <typename... Args> static shared_ptr<NodeView> make(Args
//   &&...args) {
//     return std::make_shared<NodeView>(std::forward<Args>(args)...);
//   }

//   // shared_ptr<NodeView> const &parent() const { return this->_parent; }

//   // shared_ptr<NodeView> parent() { return this->_parent; }

//   // void parent(shared_ptr<NodeView> parent) { this->_parent = parent; }

//   // void push_sub_node(shared_ptr<NodeView> n) {
//   //   this->_sub_nodes.push_back(n);
//   //   n->parent(this->shared_from_this());
//   // }

//   // std::list<shared_ptr<NodeView>> const &sub_nodes() const {
//   //   return this->_sub_nodes;
//   // }

//   // prop
//   IPoint2D const &pos() const { return this->_pos; }
//   ISize2D const &size() const { return this->_size; }
//   Rect rect() const { return Rect::Make(this->irect()); }

//   void focusable(bool v) {
//     if (this->_focusable == v) {
//       return;
//     }
//     this->_focusable = v;
//     if (v) {
//       // TODO: join focus chain
//     } else {
//       // TODO: remove focus chain
//     }
//   }

//   void focused(bool v) {
//     if (this->_focused == v) {
//       return;
//     }
//     this->_focused = v;
//     if (v) {
//       // TODO:
//     } else {
//     }
//   }

//   // render
//   SkCanvas &canvas() { return *this->_surface->getCanvas(); }

//   sk_sp<SkImage> snapshot() { return this->_surface->makeImageSnapshot(); }

//   // event
//   bool disptach_mouse_button_event(event_type<MouseButtonEvent> e) {
//     auto pos = (*e)->pos();
//     auto rect = this->rect();

//     if (this->parent() && !rect.intersect(this->parent()->rect())) {
//       return false;
//     }

//     if (!rect.contains(pos.x(), pos.y())) {
//       return false;
//     }

//     for (auto sub_node : this->sub_nodes()) {
//       auto transalte_e =
//           Event<MouseButtonEvent>::make((*e)->make_translate(this->pos()));
//       if (sub_node->disptach_mouse_button_event(transalte_e)) {
//         return true;
//       }
//     }
//     return this->post(e);
//   }

//   template <typename EventDataType, typename Callback>
//   void bind_event(Callback &&callback) {
//     auto &type_id = typeid(EventDataType);
//     if (this->is_binded<EventDataType>()) {
//       GLOG_W("%s event has been binded and will be replaced by default",
//              type_id.name());
//     }
//     this->_binded_events.emplace(type_id,
//                                  callback(on_event<EventDataType>(this)));
//   }
//   template <typename EventDataType> void unbind_event() {
//     if (this->is_binded<EventDataType>()) {
//       this->_binded_events.erase(typeid(EventDataType));
//     }
//   }

// private:

//   // prop
//   IPoint2D _pos;
//   ISize2D _size;

//   bool _focusable{false};
//   bool _focused{false};

//   // render
//   sk_sp<SkSurface> _surface;

//   // event
//   std::map<std::type_index, rx::composite_subscription> _binded_events;

//   IRect irect() const {
//     auto [x, y] = this->pos();
//     auto [w, h] = this->size();
//     return IRect::MakeXYWH(x, y, w, h);
//   }

//   // shared_ptr<NodeViewTree> tree() {
//   //   if (!this->_tree) {
//   //     throw RenderServiceError("Not connected Node view tree");
//   //   }
//   //   return this->_tree;
//   // }

//   template <typename EventDataType> bool is_binded() {
//     return this->_binded_events.count(typeid(EventDataType));
//   }

//   template <typename EventDataType> bool post(event_type<EventDataType> e) {
//     if (this->is_binded<EventDataType>()) {
//       // this->post(e->as_dynamic());
//       return true;
//     } else {
//       return false;
//     }
//   }
// };

// class NodeViewTree : public std::enable_shared_from_this<NodeViewTree> {
// public:
//   static shared_ptr<NodeViewTree> make() {
//     return std::make_shared<NodeViewTree>();
//   }

//   shared_ptr<NodeView> root() { return this->_root; }

// private:
//   shared_ptr<NodeView> _root;
// };

} // namespace my
