#include "Blend2D.hpp"

Blend2D::Blend2D() 
: BLContext()
{
}

Blend2D::~Blend2D()
{
    reset();
}

void 
Blend2D::draw(tTJSVariant target, tTJSVariant func)
{
    _target = target;

    // Layer の場合
    _width = 0;
    _height = 0;

    end();
    reset();
}


