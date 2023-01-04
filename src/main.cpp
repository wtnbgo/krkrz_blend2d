#define NOMINMAX
#include <windows.h>
#define NCBIND_UTF8
#include <ncbind.hpp>

#include <memory>

#include "Blend2D.hpp"

/**
 * ログ出力用
 */
void
message_log(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	char msg[1024];
	_vsnprintf_s(msg, 1024, _TRUNCATE, format, args);
	TVPAddLog(ttstr(msg));
	va_end(args);
}

/**
 * エラーログ出力用
 */
void
error_log(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	char msg[1024];
	_vsnprintf_s(msg, 1024, _TRUNCATE, format, args);
	TVPAddImportantLog(ttstr(msg));
	va_end(args);
}

std::unique_ptr<uint8_t[]>
openKrKrFile(const tjs_char *filename, size_t *size_out)
{
	ttstr name = filename;
	ttstr mode = "";
	iTJSBinaryStream *f = TVPCreateBinaryStreamInterfaceForRead(name, mode);
	if (f) {
		tjs_uint bufsize = f->GetSize();
		auto ret = std::make_unique<uint8_t[]>(bufsize);
		if (ret) {
			uint8_t *buf = ret.get();
			tjs_uint size = bufsize;
			while (size > 0) {
				ULONG s = f->Read(buf,size);
				size -= s;
				buf += s;
			}
			if (size_out) {
				*size_out = bufsize;
			}
			return std::move(ret);
		}
	}
	return nullptr;
}


// ----------------------------------------------------------------
// 実体型の登録
// 数値パラメータ系は配列か辞書を使えるような特殊コンバータを構築
// ----------------------------------------------------------------

// 両方自前コンバータ
#define NCB_SET_CONVERTOR_BOTH(type, convertor)\
NCB_TYPECONV_SRCMAP_SET(type, convertor<type>, true);\
NCB_TYPECONV_DSTMAP_SET(type, convertor<type>, true)

// SRCだけ自前コンバータ
#define NCB_SET_CONVERTOR_SRC(type, convertor)\
NCB_TYPECONV_SRCMAP_SET(type, convertor<type>, true);\
NCB_TYPECONV_DSTMAP_SET(type, ncbNativeObjectBoxing::Unboxing, true)

// DSTだけ自前コンバータ
#define NCB_SET_CONVERTOR_DST(type, convertor)\
NCB_TYPECONV_SRCMAP_SET(type, ncbNativeObjectBoxing::Boxing,   true); \
NCB_TYPECONV_DSTMAP_SET(type, convertor<type>, true)

/**
 * 配列かどうかの判定
 * @param var VARIANT
 * @return 配列なら true
 */
bool IsArray(const tTJSVariant &var)
{
	if (var.Type() == tvtObject) {
		iTJSDispatch2 *obj = var.AsObjectNoAddRef();
		return obj->IsInstanceOf(0, NULL, NULL, L"Array", obj) == TJS_S_TRUE;
	}
	return false;
}

// メンバ変数をプロパティとして登録
#define NCB_MEMBER_PROPERTY(name, type, membername) \
	struct AutoProp_ ## name { \
		static void ProxySet(Class *inst, type value) { inst->membername = value; } \
		static type ProxyGet(Class *inst) {      return inst->membername; } }; \
	NCB_PROPERTY_PROXY(name,AutoProp_ ## name::ProxyGet, AutoProp_ ## name::ProxySet)

#define NCB_MEMBER_PROPERTY_N(name, type) \
	struct AutoProp_ ## name { \
		static void ProxySet(Class *inst, type value) { inst->name = value; } \
		static type ProxyGet(Class *inst) {      return inst->name; } }; \
	NCB_PROPERTY_PROXY(name,AutoProp_ ## name::ProxyGet, AutoProp_ ## name::ProxySet)

// ポインタ引数型の getter を変換登録
#define NCB_ARG_PROPERTY_RO(name, type, methodname) \
	struct AutoProp_ ## name { \
		static type ProxyGet(Class *inst) { type var; inst->methodname(&var); return var; } }; \
	Property(TJS_W(# name), &AutoProp_ ## name::ProxyGet, (int)0, Proxy)

// オブジェクトなメンバーの直接参照
#define NCB_MEMBER_PROPERTY_OBJ(name, type) \
	struct AutoProp_ ## name { \
		static void ProxySet(Class *inst, tTJSVariant src) { \
			ncbTypeConvertor::SelectConvertorType<tTJSVariant, type>::Type conv; \
			conv(inst->name, src); \
		} \
		static tTJSVariant ProxyGet(Class *inst) { \
			iTJSDispatch2 *obj = ncbInstanceAdaptor<type>::CreateAdaptor(&inst->name, true); \
			return tTJSVariant(obj, obj); \
		} \
	}; \
	NCB_PROPERTY_PROXY(name,AutoProp_ ## name::ProxyGet, AutoProp_ ## name::ProxySet)

#define METHOD(Name) Method(TJS_W(#Name), &Class::Name)
#define METHOD_NAME(Name, Func) Method(TJS_W(#Name), &Class::Func)
#define METHOD_ARGS(Name, Args) Method(TJS_W(#Name), static_cast<BLResult (Class::*) Args  noexcept>(&Class::Name))
#define METHOD_ARGS_NAME(Name, Func, Args) Method(TJS_W(#Name), static_cast<BLResult (Class::*) Args noexcept>(&Class::Func))
#define METHOD_PROXY(Name, Func)  Method(TJS_W(#Name), &Func, Proxy)

// ------------------------------------------------------
// 型コンバータ登録
// ------------------------------------------------------


// ------------------------------------------------------- BLRgba32

template <class T>
struct BLRgba32Convertor {
	typedef ncbInstanceAdaptor<T> AdaptorT;
	template <typename ANYT>
	void operator ()(ANYT &adst, const tTJSVariant &src) {
		if (src.Type() == tvtObject) {
			T *obj = AdaptorT::GetNativeInstance(src.AsObjectNoAddRef());
			if (obj) {
				dst = *obj;
			} else {
				ncbPropAccessor info(src);
				if (src.Type() == tvtInteger) {
					dst = BLRgb32((tjs_int32)src);
				} else if (IsArray(src)) {
					dst = BLRgb32(info.getIntValue(1),
							      info.getIntValue(2)								  
							      info.getIntValue(3)
							      info.getIntValue(0)
								  );
				} else {
					dst = BLRgb32(info.getIntValue(L"r"),
									info.getIntValue(L"g"),
									info.getIntValue(L"b"),
									info.getIntValue(L"a"));
				}
			}
		} else {
			dst = T();
		}
		adst = ncbTypeConvertor::ToTarget<ANYT>::Get(&dst);
	}
private:
	T dst;
};

NCB_SET_CONVERTOR_DST(BLRgba32, BLRgba32Convertor);
NCB_REGISTER_CLASS_DELAY(BLRgba32, BLRgba32) {
	NCB_CONSTRUCTOR((int32_t));
};


// ------------------------------------------------------- BLPoint
template <class T>
struct BLPointConvertor {
	typedef ncbInstanceAdaptor<T> AdaptorT;
	template <typename ANYT>
	void operator ()(ANYT &adst, const tTJSVariant &src) {
		if (src.Type() == tvtObject) {
			T *obj = AdaptorT::GetNativeInstance(src.AsObjectNoAddRef());
			if (obj) {
				dst = *obj;
			} else {
				ncbPropAccessor info(src);
				if (IsArray(src)) {
					dst = BLPoint((double)info.getRealValue(0),
								 	(double)info.getRealValue(1));
				} else {
					dst = BLPoint((double)info.getRealValue(L"x"),
									 (double)info.getRealValue(L"y"));
				}
			}
		} else {
			dst = T();
		}
		adst = ncbTypeConvertor::ToTarget<ANYT>::Get(&dst);
	}
private:
	T dst;
};

NCB_SET_CONVERTOR_DST(BLPoint, BLPointConvertor);
NCB_REGISTER_CLASS_DELAY(BLPoint, BLPoint) {
	NCB_CONSTRUCTOR((double,double));
	NCB_MEMBER_PROPERTY_N(x, double);
	NCB_MEMBER_PROPERTY_N(y, double);
	NCB_METHOD_DETAIL(reset, Class, void, Class::reset, (double,double));
//	NCB_METHOD(equals);
};

// ------------------------------------------------------- BLSize
template <class T>
struct BLSizeConvertor {
	typedef ncbInstanceAdaptor<T> AdaptorT;
	template <typename ANYT>
	void operator ()(ANYT &adst, const tTJSVariant &src) {
		if (src.Type() == tvtObject) {
			T *obj = AdaptorT::GetNativeInstance(src.AsObjectNoAddRef());
			if (obj) {
				dst = *obj;
			} else {
				ncbPropAccessor info(src);
				if (IsArray(src)) {
					dst = BLSize((double)info.getRealValue(0),
								 	(double)info.getRealValue(1));
				} else {
					dst = BLSize((double)info.getRealValue(L"w"),
									 (double)info.getRealValue(L"h"));
				}
			}
		} else {
			dst = T();
		}
		adst = ncbTypeConvertor::ToTarget<ANYT>::Get(&dst);
	}
private:
	T dst;
};

NCB_SET_CONVERTOR_DST(BLSize, BLSizeConvertor);
NCB_REGISTER_CLASS_DELAY(BLSize, BLSize) {
	NCB_CONSTRUCTOR((double,double));
	NCB_MEMBER_PROPERTY_N(w, double);
	NCB_MEMBER_PROPERTY_N(h, double);
	NCB_METHOD_DETAIL(reset, Class, void, Class::reset, (double,double));
//	NCB_METHOD(equals);
};

// ------------------------------------------------------- BLRect
template <class T>
struct BLRectConvertor {
	typedef ncbInstanceAdaptor<T> AdaptorT;
	template <typename ANYT>
	void operator ()(ANYT &adst, const tTJSVariant &src) {
		if (src.Type() == tvtObject) {
			T *obj = AdaptorT::GetNativeInstance(src.AsObjectNoAddRef());
			if (obj) {
				dst = *obj;
			} else {
				ncbPropAccessor info(src);
				if (IsArray(src)) {
					dst = BLRect((double)info.getRealValue(0),
								 	(double)info.getRealValue(1),
								 	(double)info.getRealValue(2),
								 	(double)info.getRealValue(3));
				} else {
					dst = BLRect((double)info.getRealValue(L"x"),
									 (double)info.getRealValue(L"y"),
									 (double)info.getRealValue(L"w"),
									 (double)info.getRealValue(L"h"));
				}
			}
		} else {
			dst = T();
		}
		adst = ncbTypeConvertor::ToTarget<ANYT>::Get(&dst);
	}
private:
	T dst;
};

NCB_SET_CONVERTOR_DST(BLRect, BLRectConvertor);
NCB_REGISTER_CLASS_DELAY(BLRect, BLRect) {
	NCB_CONSTRUCTOR((double,double,double,double));
	NCB_MEMBER_PROPERTY_N(x, double);
	NCB_MEMBER_PROPERTY_N(y, double);
	NCB_MEMBER_PROPERTY_N(w, double);
	NCB_MEMBER_PROPERTY_N(h, double);
	NCB_METHOD_DETAIL(reset, Class, void, Class::reset, (double,double,double,double));
//	NCB_METHOD(equals);
};


// ------------------------------------------------------- BLBox
template <class T>
struct BLBoxConvertor {
	typedef ncbInstanceAdaptor<T> AdaptorT;
	template <typename ANYT>
	void operator ()(ANYT &adst, const tTJSVariant &src) {
		if (src.Type() == tvtObject) {
			T *obj = AdaptorT::GetNativeInstance(src.AsObjectNoAddRef());
			if (obj) {
				dst = *obj;
			} else {
				ncbPropAccessor info(src);
				if (IsArray(src)) {
					dst = BLBox((double)info.getRealValue(0),
								 	(double)info.getRealValue(1),
								 	(double)info.getRealValue(2),
								 	(double)info.getRealValue(3));
				} else {
					dst = BLBox((double)info.getRealValue(L"x0"),
									 (double)info.getRealValue(L"y0"),
									 (double)info.getRealValue(L"x1"),
									 (double)info.getRealValue(L"y1"));
				}
			}
		} else {
			dst = T();
		}
		adst = ncbTypeConvertor::ToTarget<ANYT>::Get(&dst);
	}
private:
	T dst;
};

NCB_SET_CONVERTOR_DST(BLBox, BLBoxConvertor);
NCB_REGISTER_CLASS_DELAY(BLBox, BLBox) {
	NCB_CONSTRUCTOR((double,double,double,double));
	NCB_MEMBER_PROPERTY_N(x0, double);
	NCB_MEMBER_PROPERTY_N(y0, double);
	NCB_MEMBER_PROPERTY_N(x1, double);
	NCB_MEMBER_PROPERTY_N(y1, double);
	NCB_METHOD_DETAIL(reset, Class, void, Class::reset, (double,double,double,double));
//	NCB_METHOD(equals);
};

// ------------------------------------------------------- BLFontMetrics

NCB_REGISTER_CLASS_DELAY(BLFontMetrics, BLFontMetrics) {
	NCB_CONSTRUCTOR(());
	NCB_MEMBER_PROPERTY_N(size, float);
	NCB_MEMBER_PROPERTY_N(ascent, float);
	NCB_MEMBER_PROPERTY_N(vAscent, float);
	NCB_MEMBER_PROPERTY_N(descent, float);
	NCB_MEMBER_PROPERTY_N(vDescent, float);

	NCB_MEMBER_PROPERTY_N(lineGap, float);
	NCB_MEMBER_PROPERTY_N(xHeight, float);
	NCB_MEMBER_PROPERTY_N(capHeight, float);
	NCB_MEMBER_PROPERTY_N(xMin, float);
	NCB_MEMBER_PROPERTY_N(yMin, float);
	NCB_MEMBER_PROPERTY_N(xMax, float);
	NCB_MEMBER_PROPERTY_N(yMax, float);
	NCB_MEMBER_PROPERTY_N(underlinePosition, float);
	NCB_MEMBER_PROPERTY_N(underlineThickness, float);
	NCB_MEMBER_PROPERTY_N(strikethroughPosition, float);
	NCB_MEMBER_PROPERTY_N(strikethroughThickness, float);
	NCB_METHOD(reset);
};

// ------------------------------------------------------- BLTextMetrics

NCB_REGISTER_CLASS_DELAY(BLTextMetrics, BLTextMetrics) {
	NCB_CONSTRUCTOR(());
	NCB_MEMBER_PROPERTY_OBJ(advance, BLPoint);
	NCB_MEMBER_PROPERTY_OBJ(leadingBearing, BLPoint);
	NCB_MEMBER_PROPERTY_OBJ(trailingBearing, BLPoint);
	NCB_MEMBER_PROPERTY_OBJ(boundingBox, BLBox);
	NCB_METHOD(reset);
};

// ------------------------------------------------------- BLMatrix2D
template <class T>
struct BLMatrix2DConvertor {
	typedef ncbInstanceAdaptor<T> AdaptorT;
	template <typename ANYT>
	void operator ()(ANYT &adst, const tTJSVariant &src) {
		if (src.Type() == tvtObject) {
			T *obj = AdaptorT::GetNativeInstance(src.AsObjectNoAddRef());
			if (obj) {
				dst = *obj;
			} else {
				ncbPropAccessor info(src);
				if (IsArray(src)) {
					dst = BLMatrix2D((double)info.getRealValue(0),
								 	(double)info.getRealValue(1),
								 	(double)info.getRealValue(2),
								 	(double)info.getRealValue(3),
								 	(double)info.getRealValue(4),
								 	(double)info.getRealValue(5));
				} else {
					dst = BLMatrix2D((double)info.getRealValue(L"m00"),
									 (double)info.getRealValue(L"m01"),
									 (double)info.getRealValue(L"m10"),
									 (double)info.getRealValue(L"m11"),
								 	(double)info.getRealValue(L"m20"),
								 	(double)info.getRealValue(L"m21"));
				}
			}
		} else {
			dst = T();
		}
		adst = ncbTypeConvertor::ToTarget<ANYT>::Get(&dst);
	}
private:
	T dst;
};

NCB_SET_CONVERTOR_DST(BLMatrix2D, BLMatrix2DConvertor);
NCB_REGISTER_CLASS_DELAY(BLMatrix2D, BLMatrix2D) {
	NCB_CONSTRUCTOR((double,double,double,double,double,double));
	NCB_MEMBER_PROPERTY_N(m00, double);
	NCB_MEMBER_PROPERTY_N(m01, double);
	NCB_MEMBER_PROPERTY_N(m10, double);
	NCB_MEMBER_PROPERTY_N(m11, double);
	NCB_MEMBER_PROPERTY_N(m20, double);
	NCB_MEMBER_PROPERTY_N(m21, double);
	NCB_METHOD_DETAIL(reset, Class, void, Class::reset, ());
	NCB_METHOD_DETAIL(translate, Class, BLResult, Class::translate, (double,double));
	NCB_METHOD_DETAIL(scale, Class, BLResult, Class::scale, (double,double));
	NCB_METHOD_DETAIL(skew, Class, BLResult, Class::skew, (double,double));
	NCB_METHOD_DETAIL(rotate, Class, BLResult, Class::rotate, (double));
	NCB_METHOD_DETAIL(postTranslate, Class, BLResult, Class::postTranslate, (double,double));
	NCB_METHOD_DETAIL(postScale, Class, BLResult, Class::postScale, (double,double));
	NCB_METHOD_DETAIL(postSkew, Class, BLResult, Class::postSkew, (double,double));
	NCB_METHOD_DETAIL(postRotate, Class, BLResult, Class::postRotate, (double));
	NCB_METHOD_DETAIL(transform, Class, BLResult, Class::transform, (const BLMatrix2D&));
	NCB_METHOD_DETAIL(postTransform, Class, BLResult, Class::postTransform, (const BLMatrix2D&));
	NCB_METHOD_DETAIL(invert, Class, BLResult, Class::invert, ());
};


// ------------------------------------------------------
// ベースクラス
// ------------------------------------------------------

#define ENUM(n) Variant(#n, (int)n)
#define NCB_SUBCLASS_NAME(name) NCB_SUBCLASS(name, name)

NCB_REGISTER_CLASS(Blend2D)
{
	NCB_METHOD(drawToLayer);
	NCB_METHOD(addFont);
}

NCB_REGISTER_CLASS(BLFontFaceInfo)
{
	NCB_CONSTRUCTOR(());
}

bool face_load(BLFontFace *face, const char *family) 
{
	return Blend2D::loadFace(family, *face);
}

bool face_loadfile(BLFontFace *face, const tjs_char *file) 
{
	return Blend2D::loadFaceFile(file, *face);
}

NCB_REGISTER_CLASS(BLFontFace)
{
	NCB_CONSTRUCTOR(());

	METHOD_PROXY(load, face_load);
	METHOD_PROXY(loadFile, face_loadfile);

	METHOD(reset);
	METHOD_ARGS(assign, (const BLFontFace&));
	METHOD(isValid);
	METHOD(empty);
	METHOD(equals);

	METHOD(weight);
	METHOD(stretch);
	METHOD(style);
	METHOD(faceInfo);
	METHOD(faceType);
	METHOD(outlineType);
	METHOD(glyphCount);

	METHOD(faceIndex);
	METHOD(faceFlags);

	METHOD(hasFaceFlag);
	METHOD(hasTypographicNames);
	METHOD(hasTypographicMetrics);
	METHOD(hasCharToGlyphMapping);
	METHOD(hasHorizontalMetrics);
	METHOD(hasVerticalMetrics);
	METHOD(hasHorizontalKerning);
	METHOD(hasVerticalKerning);
	METHOD(hasOpenTypeFeatures);
	METHOD(hasPanoseData);
	METHOD(hasUnicodeCoverage);
	METHOD(hasBaselineYAt0);
	METHOD(hasLSBPointXAt0);
	METHOD(hasVariationSequences);
	METHOD(hasOpenTypeVariations);
	METHOD(isSymbolFont);
	METHOD(isLastResortFont);
	METHOD(diagFlags);
	METHOD(uniqueId);
	METHOD(fullName);
	METHOD(familyName);
	METHOD(subfamilyName);
	METHOD(postScriptName);
	METHOD(unitsPerEm);
}

NCB_REGISTER_CLASS(BLGlyphMappingState)
{
	NCB_CONSTRUCTOR(());
}

NCB_REGISTER_CLASS(BLGlyphRun)
{
	NCB_CONSTRUCTOR(());
}

BLResult setText(BLGlyphBuffer *gb, const char *text)
{
  return gb->setUtf8Text(text);
}

NCB_REGISTER_CLASS(BLGlyphBuffer)
{
	NCB_CONSTRUCTOR(());
	METHOD_PROXY(setText, setText);
	METHOD(glyphRun);
}

NCB_REGISTER_CLASS(BLBitSet)
{
	NCB_CONSTRUCTOR(());
}

NCB_REGISTER_CLASS(BLFontMatrix)
{
	NCB_CONSTRUCTOR(());
}

NCB_REGISTER_CLASS(BLFontDesignMetrics)
{
	NCB_CONSTRUCTOR(());
}

bool font_load(BLFont *font, const char *family, float size) 
{
	return Blend2D::loadFont(family, size, *font);
}

bool font_loadFace(BLFont *font, BLFontFace *face, float size) 
{
	return font->createFromFace(*face, size) == 0;
}

tjs_uintptr_t nk_font_userdata(BLFont *font) {
	return (tjs_uintptr_t)font;
};

extern float nk_text_width(BLFont *font, float h, const char *text, int len);

tjs_uintptr_t nk_font_width_func(BLFont *font) 
{
	return (tjs_uintptr_t)(void*)&nk_text_width;
}

float nk_font_size(BLFont *font)
{
	return font->size();
}

NCB_REGISTER_CLASS(BLFont)
{
	NCB_CONSTRUCTOR(());
	METHOD_PROXY(load, font_load);
	METHOD_PROXY(loadFace, font_loadFace);

//	METHOD(reset);
	METHOD(swap);
	METHOD_ARGS(assign, (const BLFont &));
	METHOD(isValid);
	METHOD(empty);
	METHOD(equals);
	METHOD(faceType);
	METHOD(faceFlags);
	METHOD(size);
	METHOD(unitsPerEm);
	METHOD(face);
	METHOD(weight);
	METHOD(stretch);
	METHOD(style);
	METHOD(matrix);
	METHOD(metrics);
	METHOD(designMetrics);

	// メソッドの引数の型補正のために強制キャスト
	// BLGlyphBufferCore → BLGlyphBuffer
	Method(TJS_W("shape"), (BLResult (Class::*)(const BLGlyphBuffer&) const)(BLResult (Class::*)(const BLGlyphBufferCore&) const)&Class::shape);
	Method(TJS_W("mapTextToGlyphs"), (BLResult (Class::*)(BLGlyphBuffer&) noexcept)(BLResult (Class::*)(BLGlyphBufferCore&) const noexcept)&Class::mapTextToGlyphs);
	Method(TJS_W("positionGlyphs"), (BLResult (Class::*)(BLGlyphBuffer&, uint32_t) const noexcept)(BLResult (Class::*)(BLGlyphBufferCore&, uint32_t) const noexcept)&Class::positionGlyphs);
	Method(TJS_W("applyKerning"), (BLResult (Class::*)(BLGlyphBuffer&) const noexcept)(BLResult (Class::*)(BLGlyphBuffer&) const noexcept)&Class::applyKerning);
	Method(TJS_W("applyGSub"), (BLResult (Class::*)(BLGlyphBuffer&, BLBitSet&) const noexcept)(BLResult (Class::*)(BLGlyphBufferCore&, BLBitSetCore&) const noexcept)&Class::applyGSub);
	Method(TJS_W("applyGPos"), (BLResult (Class::*)(BLGlyphBuffer&, BLBitSet&) const noexcept)(BLResult (Class::*)(BLGlyphBufferCore&, BLBitSetCore&) const noexcept)&Class::applyGPos);
	Method(TJS_W("getTextMetrics"), (BLResult (Class::*)(BLGlyphBuffer&, BLTextMetrics&) const noexcept)(BLResult (Class::*)(BLGlyphBufferCore&, BLTextMetrics&) const noexcept)&Class::getTextMetrics);

	// nuklear 用プロパティ
	Property(TJS_W("nk_userdata"), &nk_font_userdata, (int)0, Proxy);
	Property(TJS_W("nk_font_height"), &nk_font_size, (int)0, Proxy);
	Property(TJS_W("nk_font_width_func"), &nk_font_width_func, (int)0, Proxy);
}

NCB_REGISTER_CLASS(BLStrokeOptions)
{
	NCB_CONSTRUCTOR(());
}

NCB_REGISTER_CLASS(BLPath)
{
	NCB_CONSTRUCTOR(());
}

tjs_uintptr_t nk_image_userdata(BLImage *image) {
	return (tjs_uintptr_t)image;
};

NCB_REGISTER_CLASS(BLImage)
{
	NCB_CONSTRUCTOR(());

	// nuklear 用プロパティ
	Property(TJS_W("nk_userdata"), &nk_image_userdata, (int)0, Proxy);
	METHOD_NAME(nk_image_width, width);
	METHOD_NAME(nk_image_height, height);
}

tjs_uintptr_t nk_context_userdata(BLContext *ctx) {
	return (tjs_uintptr_t)ctx;
};

extern void nk_render(BLContext *ctx, const struct nk_command *cmd);

tjs_uintptr_t nk_render_func(BLContext *ctx) 
{
    return (tjs_uintptr_t)(void*)&nk_render;
};


BLResult fillText(BLContext *ctx, double x, double y, const BLFont &font, const char *text)
{
  return ctx->fillUtf8Text(BLPoint(x, y), font, text);
}

BLResult fillGlyphBuffer(BLContext *ctx, double x, double y, const BLFont &font, const BLGlyphBuffer &gb)
{
  return ctx->fillGlyphRun(BLPoint(x, y), font, gb.glyphRun());
}

BLResult fillGlyphRun(BLContext *ctx, double x, double y, const BLFont &font, const BLGlyphRun &glyphRun)
{
  return ctx->fillGlyphRun(BLPoint(x, y), font, glyphRun);
}

BLResult strokeText(BLContext *ctx, double x, double y, const BLFont &font, const char *text)
{
  return ctx->strokeUtf8Text(BLPoint(x, y), font, text);
}

BLResult strokeGlyphBuffer(BLContext *ctx, double x, double y, const BLFont &font, const BLGlyphBuffer &gb)
{
  return ctx->strokeGlyphRun(BLPoint(x, y), font, gb.glyphRun());
}

BLResult strokeGlyphRun(BLContext *ctx, double x, double y, const BLFont &font, const BLGlyphRun &glyphRun)
{
  return ctx->strokeGlyphRun(BLPoint(x, y), font, glyphRun);
}

BLResult blitImage(BLContext *ctx, double x, double y, double w, double h, const BLImage &image, int sx, int sy, int sw, int sh)
{
  BLRect dst(x,y,w,h);
  BLRectI srcArea(sw,sy,sw,sh);
  return ctx->blitImage(dst, image, srcArea);
}


NCB_REGISTER_CLASS(BLContext)
{
	NCB_CONSTRUCTOR(());

	METHOD(targetSize);
	METHOD(targetWidth);
	METHOD(targetHeight);
	//METHOD(targetImage);

	METHOD(contextType);
	METHOD(isValid);
	METHOD(reset);
	METHOD_ARGS(assign, (const BLContext&));

	//METHOD(begin);
	//METHOD(end);

	METHOD(flush);

	METHOD_ARGS(save, ());
	METHOD_ARGS(restore, ());

	METHOD(metaMatrix);
	METHOD(userMatrix);

	METHOD(setMatrix);
	METHOD(resetMatrix);

	METHOD_ARGS(translate, (double, double));
	METHOD_ARGS(scale, (double, double));
	METHOD_ARGS(skew, (double, double));
	METHOD_ARGS(rotate, (double));
	METHOD_ARGS_NAME(rotateCenter, rotate, (double, double, double));
	METHOD(transform);
	METHOD_ARGS(postTranslate, (double, double));
	METHOD_ARGS(postScale, (double, double));
	METHOD_ARGS(postSkew, (double, double));
	METHOD_ARGS(postRotate, (double));
	METHOD_ARGS_NAME(postRotateCenter, postRotate, (double, double, double));
	METHOD(postTransform);

	METHOD(userToMeta);

	// Hints

	METHOD(setRenderingQuality);
	METHOD(setGradientQuality);
	METHOD(setPatternQuality);

	METHOD(flattenMode);
	METHOD(setFlattenMode);

	METHOD(flattenTolerance);
	METHOD(setFlattenTolerance);

	METHOD(compOp);
	METHOD(setCompOp);

	METHOD(globalAlpha);
	METHOD(setGlobalAlpha);

	METHOD(styleType);
	//METHOD_ARGS(setStyle, (uint32_t, const BLStyleCore&));

	METHOD(styleAlpha);
	METHOD(setStyleAlpha);

	METHOD_ARGS(setFillStyle, (const BLRgba32&));
	METHOD(getFillStyle);
	METHOD(fillAlpha);
	METHOD(setFillAlpha);
	METHOD(fillRule);
	METHOD(setFillRule);

	METHOD_ARGS(setStrokeStyle, (const BLRgba32&));

	METHOD(strokeWidth);
	METHOD(strokeMiterLimit);
	METHOD(strokeJoin);
	METHOD(strokeStartCap);
	METHOD(strokeEndCap);
	METHOD(strokeDashOffset);
	//METHOD(strokeDashArray)
	METHOD(strokeTransformOrder);
	METHOD(strokeOptions);

	METHOD(setStrokeWidth);
	METHOD(setStrokeMiterLimit);
	METHOD(setStrokeJoin);
	METHOD(setStrokeStartCap);
	METHOD(setStrokeEndCap);
	METHOD(setStrokeCaps);
	METHOD(setStrokeDashOffset);
	METHOD(setStrokeTransformOrder);
	METHOD(setStrokeOptions);

	METHOD(strokeAlpha);
	METHOD(setStrokeAlpha);

	METHOD(restoreClipping);
	METHOD_ARGS(clipToRect, (double, double, double, double));

	METHOD(clearAll);
	METHOD_ARGS(clearRect, (double, double, double, double));

	//METHOD(fillGeometry);
	METHOD(fillAll);
	METHOD_ARGS(fillBox, (double, double, double, double));
	METHOD_ARGS(fillRect, (double, double, double, double));
	METHOD_ARGS(fillCircle, (double, double, double));
	METHOD_ARGS(fillEllipse, (double, double, double, double));
	METHOD_ARGS(fillRoundRect, (double, double, double, double, double));
	METHOD_ARGS(fillChord, (double, double, double, double, double, double));
	METHOD_ARGS(fillPie, (double, double, double, double, double));
	METHOD_ARGS(fillTriangle, (double, double, double, double, double, double));
	//METHOD_ARGS(fillPolygon, )
	//METHOD_ARGS(fillBoxArray, )
	//METHOD_ARGS(fillRectArray, )
	//METHOD(fillRegion);
	METHOD(fillPath);
	METHOD_PROXY(fillText, fillText);
	METHOD_PROXY(fillGlyphBuffer, fillGlyphBuffer);
	METHOD_PROXY(fillGlyphRun, fillGlyphRun);

	//METHOD(strokeGeometry);
	METHOD_ARGS(strokeBox, (double, double, double, double));
	METHOD_ARGS(strokeRect, (double, double, double, double));
	METHOD_ARGS(strokeCircle, (double, double, double));
	METHOD_ARGS(strokeEllipse, (double, double, double, double));
	METHOD_ARGS(strokeRoundRect, (double, double, double, double, double));
	METHOD_ARGS(strokeChord, (double, double, double, double, double, double));
	METHOD_ARGS(strokePie, (double, double, double, double, double));
	METHOD_ARGS(strokeTriangle, (double, double, double, double, double, double));
	//METHOD_ARGS(strokePolygon, )
	//METHOD_ARGS(strokeBoxArray, )
	//METHOD_ARGS(strokeRectArray, )
	//METHOD(strokeRegion);
	METHOD(strokePath);
	METHOD_PROXY(strokeText, strokeText);
	METHOD_PROXY(strokeGlyphBuffer, strokeGlyphBuffer);
	METHOD_PROXY(strokeGlyphRun, strokeGlyphRun);

	METHOD_PROXY(blitImage, blitImage);

	// nuklear 用プロパティ
	Property(TJS_W("nk_userdata"), &nk_context_userdata, (int)0, Proxy);
	Property(TJS_W("nk_render_func"), &nk_render_func, (int)0, Proxy);
}

tTJSVariant toVariant(BLContext *obj, bool sticky)
{
	iTJSDispatch2 *dispatch = ncbInstanceAdaptor<BLContext>::CreateAdaptor(obj, sticky);
	return tTJSVariant(dispatch, dispatch);
}
