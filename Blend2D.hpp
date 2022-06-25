#pragma once

#include <Windows.h>
#include <tp_stub.h>
#include <blend2d.h>

class Blend2D : public BLContext {

public:
  Blend2D();
  virtual ~Blend2D();

  void draw(tTJSVariant target, tTJSVariant func);

private:
  tTJSVariant _target;
  int _width;
  int _height;
};


