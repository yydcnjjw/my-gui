#include <gtest/gtest.h>

#include <render/node.hpp>

TEST(RenderNodeTest, node_build) {

  auto root = my::Node::make_root("head");

  root->append_child(my::Node::make("1"));
  root->append_child(my::Node::make("2"));

  {
    auto node_3 = my::Node::make("3");
    root->append_child(node_3);

    node_3->append_child(my::Node::make("3-1"));
    node_3->append_child(my::Node::make("3-2"));
    node_3->append_child(my::Node::make("3-3"));
  }

  {
    auto node_4 = my::Node::make("4");
    root->append_child(node_4);

    {
      auto node_4_1 = my::Node::make("4-1");
      node_4->append_child(node_4_1);
      node_4_1->append_child(my::Node::make("4-1-1"));
      node_4_1->append_child(my::Node::make("4-1-2"));
    }
  }

  root->post_order([](my::shared_ptr<my::Node> const &node) {
    for (int i = 0; i < node->depth(); ++i)
      std::cout << " ";
    std::cout << node->name() << std::endl;
  });
}
