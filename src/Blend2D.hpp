#pragma once

#include <tp_stub.h>
#include <blend2d.h>

struct Blend2D {

  /**
   * @brief Blend2D が参照するフォントファイルを追加する。
   * @param file フォントファイル
   */
  static void addFont(const tjs_char *file);

  /**
   * @brief フォントオブジェクトを取得
   * 
   * @param family ファミリー名
   * @param size フォントサイズ
   * @return BLFont* 
   */
  static bool loadFont(const char *family, float size, BLFont &font);

  /**
   * @brief レイヤに対する描画処理実行
   * 
   * @param layer レイヤオブジェクト
   * @param func 
   */
  static void drawToLayer(tTJSVariant layer, tTJSVariant func);
};

tTJSVariant toVariant(BLContext *ctx, bool sticky=true);
