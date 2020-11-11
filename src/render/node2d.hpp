#pragma once

#include <core/core.hpp>
#include <window/window.hpp>

#include <render/exception.hpp>
#include <render/type.hpp>

namespace my {
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

    template <typename... Args>
    static shared_ptr<Node2D> make(Args &&... args) {
        return std::make_shared<Node2D>(std::forward<Args>(args)...);
    }
#ifdef MY_DEBUG
#define CHECK_RUN_AT_RENDER_THREAD                                             \
    this->check_run_at_render_thread(this->shared_from_this());
#define CHECK_RUN_AT_RENDER_THREAD_WITH_NODE(n)                                \
    this->check_run_at_render_thread(n);
#else
#define CHECK_RUN_AT_RENDER_THREAD
#define CHECK_RUN_AT_RENDER_THREAD_WITH_NODE(n)
#endif // MY_DEBUG

    // render
    IPoint2D const &pos() const { return this->_pos; }
    ISize2D const &size() const { return this->_size; }
    Rect rect() const { return Rect::Make(this->irect()); }

    SkCanvas &canvas() {
        CHECK_RUN_AT_RENDER_THREAD
        return *this->_surface->getCanvas();
    }

    sk_sp<SkImage> snapshot() {
        CHECK_RUN_AT_RENDER_THREAD
        return this->_surface->makeImageSnapshot();
    }

    shared_ptr<Node2D> const &parent() const { return this->_parent; }

    shared_ptr<Node2D> parent() { return this->_parent; }
    void parent(shared_ptr<Node2D> parent) {
        CHECK_RUN_AT_RENDER_THREAD_WITH_NODE(parent)
        this->_parent = parent;
    }

    void add_sub_node(shared_ptr<Node2D> n) {
        CHECK_RUN_AT_RENDER_THREAD
        n->parent(this->shared_from_this());
        this->_sub_nodes.push_back(n);
    }

    std::list<shared_ptr<Node2D>> const &sub_nodes() const {
        return this->_sub_nodes;
    }

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
            auto transalte_e = Event<MouseButtonEvent>::make(
                (*e)->make_translate(this->pos()));
            if (sub_node->disptach_mouse_button_event(transalte_e)) {
                return true;
            }
        }
        return this->post(e);
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
    shared_ptr<Node2D> _parent;
    std::list<shared_ptr<Node2D>> _sub_nodes;

    IPoint2D _pos;
    ISize2D _size;

    sk_sp<SkSurface> _surface;

    subject_dynamic_event_type _subject;
    std::map<std::type_index, rx::composite_subscription> _binded_events;

    IRect irect() const {
        auto [x, y] = this->pos();
        auto [w, h] = this->size();
        return IRect::MakeXYWH(x, y, w, h);
    }

    observable_type event_source() override {
        return this->_subject.get_observable();
    }

    template <typename EventDataType> bool is_binded() {
        return this->_binded_events.count(typeid(EventDataType));
    }

    template <typename EventDataType> bool post(event_type<EventDataType> e) {
        if (this->is_binded<EventDataType>()) {
            this->_subject.get_subscriber().on_next(e->as_dynamic());
            return true;
        } else {
            return false;
        }
    }

#ifdef MY_DEBUG
    inline void check_run_at_render_thread(shared_ptr<Node2D>);
#endif // MY_DEBUG
};

class FocusHandler : public Observer {
    void subscribe(MY_UNUSED Observable *o) override {}
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

#ifdef MY_DEBUG
    void reset_render_thread_info() { this->_render_thread_info.reset(); }
    void render_thread_info(thread_info const &t) {
        this->_render_thread_info = t;
    }

    std::optional<thread_info> &render_thread_info() {
        return this->_render_thread_info;
    }

#endif // MY_DEBUG

  private:
    using Node2D::make;
    using Node2D::parent;

#ifdef MY_DEBUG
    std::optional<thread_info> _render_thread_info;
#endif // MY_DEBUG
};

void Node2D::check_run_at_render_thread(shared_ptr<Node2D> node) {
    while (node->parent()) {
        node = node->parent();
    }
    auto root = std::dynamic_pointer_cast<RootNode2D>(node);

    if (root) {
        // if bind to root
        auto thread_info = root->render_thread_info();
        if (thread_info && !(*thread_info).at_this_thread()) {
            throw RenderServiceError("Node2D ui operations must render thread");
        }
    }
}

} // namespace my
