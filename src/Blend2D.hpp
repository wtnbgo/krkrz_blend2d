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
   * @brief フェイスオブジェクトにフォントを読み込み
   * 
   * @param family ファミリー名
   * @param fontFace 格納先
   * @return true  成功
   * @return false 失敗
   */
  static bool loadFace(const char *family, BLFontFace &fontFace);

  static bool loadFaceFile(const tjs_char *file, BLFontFace &fontFace);

  /**
   * @brief フォントオブジェクトを取得
   * 
   * @param family ファミリー名
   * @param size フォントサイズ
   * @param font 格納先
   * @return true  成功
   * @return false 失敗
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
