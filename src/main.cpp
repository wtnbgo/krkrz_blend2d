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

// ------------------------------------------------------
// 型コンバータ登録
// ------------------------------------------------------


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
	NCB_METHOD_PROXY(load, face_load);
	NCB_METHOD_PROXY(loadFile, face_loadfile);
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
	NCB_METHOD_PROXY(load, font_load);
	NCB_METHOD_PROXY(loadFace, font_loadFace);

	// nuklear 用プロパティ
	Property(TJS_W("nk_userdata"), &nk_font_userdata, (int)0, Proxy);
	Property(TJS_W("nk_font_height"), &nk_font_size, (int)0, Proxy);
	Property(TJS_W("nk_font_width_func"), &nk_font_width_func, (int)0, Proxy);
}

tjs_uintptr_t nk_image_userdata(BLImage *image) {
	return (tjs_uintptr_t)image;
};

NCB_REGISTER_CLASS(BLImage)
{
	NCB_CONSTRUCTOR(());

	// nuklear 用プロパティ
	Property(TJS_W("nk_userdata"), &nk_image_userdata, (int)0, Proxy);
//	Property(TJS_W("nk_image_width"), &BLImage::width, (int)0);
//	Property(TJS_W("nk_image_height"), &BLImage::height, (int)0);
}

tjs_uintptr_t nk_context_userdata(BLContext *ctx) {
	return (tjs_uintptr_t)ctx;
};

extern void nk_render(BLContext *ctx, const struct nk_command *cmd);

tjs_uintptr_t nk_render_func(BLContext *ctx) 
{
    return (tjs_uintptr_t)(void*)&nk_render;
};

NCB_REGISTER_CLASS(BLContext)
{
	NCB_CONSTRUCTOR(());

	// nuklear 用プロパティ
	Property(TJS_W("nk_userdata"), &nk_context_userdata, (int)0, Proxy);
	Property(TJS_W("nk_render_func"), &nk_render_func, (int)0, Proxy);
}

tTJSVariant toVariant(BLContext *obj, bool sticky)
{
	iTJSDispatch2 *dispatch = ncbInstanceAdaptor<BLContext>::CreateAdaptor(obj, sticky);
	return tTJSVariant(dispatch, dispatch);
}
