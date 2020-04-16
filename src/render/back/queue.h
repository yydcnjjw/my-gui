#pragma once

namespace my {
namespace render {

enum QueueType { QUEUE_GRAPHICS, QUEUE_COMPUTE, QUEUE_TRANSFER, QUEUE_PRESENT };

class Queue {
  public:
    virtual void present() = 0;
    virtual void submit() = 0;
    virtual void wait_idle() = 0;
};
} // namespace render
} // namespace my
