#define NOMINMAX
#include <windows.h>
#define NCBIND_UTF8
#include <memory>
#include <ncbind.hpp>

#include "Blend2D.hpp"

std::unique_ptr<uint8_t[]> openKrKrFile(const tjs_char *filename,
                                        size_t *size_out);
// blend2dフォントマネージャ
static BLFontManager fontManager;

// フォントデータを登録
void Blend2D::addFont(const tjs_char *file) 
{
  ttstr filename = TVPGetPlacedPath(file);
  if (filename.length()) {
    size_t size;
    auto data = openKrKrFile(filename.c_str(), &size);
    if (data) {
      BLArray<uint8_t> dat;
      dat.assignExternalData(data.get(), size, size, BL_DATA_ACCESS_READ);
      BLFontData fontData;
      fontData.createFromData(dat);
      for (int i = 0; i < fontData.faceCount(); i++) {
        BLFontFace fontFace;
        fontFace.createFromData(fontData, i);
        fontManager.addFace(fontFace);
      }
    }
  }
}

// フォントデータをファミリー名指定して読み込み
bool Blend2D::loadFont(const char *family, float size, BLFont &font)
{
  BLArray<BLFontFace> fontFaces;
  fontManager.queryFacesByFamilyName(family, fontFaces);
  if (fontFaces.size() > 0) {
    font.createFromFace(fontFaces[0], size);
    return true;
  }
  return false;
}

static tjs_uint32 updateHint;

void Blend2D::drawToLayer(tTJSVariant layer, tTJSVariant callback) {
  if (!layer.AsObjectNoAddRef()->IsInstanceOf(0, 0, 0, L"Layer", NULL)) {
    TVPThrowExceptionMessage(L"not layer");
  }

  ncbPropAccessor p(layer);
  int width = p.getIntValue(TJS_W("imageWidth"));
  int height = p.getIntValue(TJS_W("imageHeight"));
  int pitch = p.getIntValue(TJS_W("mainImageBufferPitch"));
  void *buf = (void *)p.getIntValue(TJS_W("mainImageBufferForWrite"));

  bool bottomUp = false;
  if (pitch < 0) {
    bottomUp = true;
    buf = (unsigned char *)buf + pitch * (height - 1);
    pitch = -pitch;
  }

  BLImage img;
  img.createFromData(width, height, BL_FORMAT_PRGB32, buf, pitch);
  BLContext ctx(img);
  ctx.setCompOp(BL_COMP_OP_SRC_COPY);

  if (callback.Type() == tvtObject) {
    tTJSVariant canvasObj = toVariant(&ctx);
    tTJSVariant widthObj = width;
    tTJSVariant heightObj = height;
    tTJSVariant bottomUpObj = bottomUp;
    tTJSVariant *vars[] = {&canvasObj, &widthObj, &heightObj, &bottomUpObj};
    callback.AsObjectClosureNoAddRef().FuncCall(0, 0, 0, NULL, 4, vars, 0);

    // レイヤ更新 帰り値で処理が妥当か？
    p.FuncCall(0, TJS_W("update"), &updateHint, NULL, 0, 0, width, height);
  }
}
