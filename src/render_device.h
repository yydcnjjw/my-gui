#pragma once

class RenderDevice {
 public:
    RenderDevice() = default;
    virtual ~RenderDevice() = default;

    virtual void texture_create();
};
