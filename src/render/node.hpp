#pragma once

#include <core/core.hpp>

#include <render/exception.hpp>
#include <render/type.hpp>
#include <util/uuid.hpp>

#include <tree.hh>

namespace my {

// class Node:
//   修改操作(针对子树)
//   只读操作
//   保存 RootNode 实例(用于只读操作整个树)
//   保存 node pos (nodeTreeIter)
// class RootNode
//   读写操作树

template <typename T> class TreeNode {
  using data_type = T;
  using self_type = TreeNode<data_type>;
  using ptr_type = shared_ptr<self_type>;
  using tree_type = tree<data_type>;
  using tree_iter_type = typename tree_type::iterator;
  using tree_ptr_type = shared_ptr<tree_type>;

public:
  TreeNode() {}

  // TreeNode(tree_ptr_type tree, tree_iter_type pos) : _tree(tree), _pos(pos)
  // {}

  data_type const &parent() const {
    return *this->_tree->parent(this->_pos);
  }
  
  void append_child(data_type child) {
    auto child_pos = this->_tree->append_child(this->_pos, child);
    child->set_tree_node(this->_tree, child_pos);
  }

  void set_tree_node(tree_ptr_type tree, tree_iter_type pos) {
    BOOST_ASSERT_MSG(!this->_tree, "node cannot have multiple parent nodes");

    this->_tree = tree;
    this->_pos = pos;
  }

  void as_root(data_type head) {
    auto tree = std::make_shared<tree_type>();
    auto pos = tree->set_head(head);
    this->set_tree_node(tree, pos);
  }

  template <typename Callback> void post_order(Callback &&cb) {
    auto const &tree = this->_tree;
    tree_iter_type it = tree->begin();
    while (it != tree->end())
      cb(*it++);
  }

  bool is_root() const { return this->_tree->is_head(this->iter); }

  T const &root() const { return this->_tree->head->data; };

  int depth() const { return this->_tree->depth(this->_pos); }

private:
  tree_ptr_type _tree;
  tree_iter_type _pos;
};

class Node : public std::enable_shared_from_this<Node>,
             public TreeNode<shared_ptr<Node>> {
  using self_type = Node;
  using ptr_type = shared_ptr<Node>;
  using tree_node_type = TreeNode<shared_ptr<Node>>;

public:
  virtual ~Node() = default;

  static ptr_type make() { return shared_ptr<Node>(new Node()); }

  static ptr_type make_root() {
    auto root = Node::make();
    root->as_root(root);
    return root;
  }

  void append_child(ptr_type child) { tree_node_type::append_child(child); }

  std::string const &name() const { return this->_name; }

  void name(std::string const &name) { this->_name = name; }

  uuid const &id() const { return this->_id; }

protected:
  Node() : _id{uuid_gen()} {}

private:
  using tree_node_type::append_child;

  // void id(std::string const &id) { this->_id = id; }

  uuid _id;
  std::string _name;
};

} // namespace my
