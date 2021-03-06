#pragma once

#include <core/type.hpp>

#include <skia/include/core/SkBitmap.h>
#include <skia/include/core/SkCanvas.h>
#include <skia/include/core/SkFont.h>
#include <skia/include/core/SkSurface.h>
#include <skia/include/core/SkRegion.h>

namespace my {

using IRect = SkIRect;
using IPoint2D = SkIPoint;
using ISize2D = SkISize;

using Rect = SkRect;
using Point2D = SkPoint;
using Size2D = SkSize;

using Region = SkRegion;

using Matrix = SkMatrix;
// using Canvas = SkCanvas;
// using Surface = SkSurface;
// using Bitmap = SkBitmap;
// using ImageInfo = SkImageInfo;

} // namespace my
