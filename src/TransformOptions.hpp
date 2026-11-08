#pragma once

struct TransformOptions
{
  bool skipRgb = false;
  TransformOptions(const Shared * const shared) {
    skipRgb = shared->GetOptionSkipRGB();
  }
};
