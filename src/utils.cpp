#define NOMINMAX
#include <windows.h>
#define NCBIND_UTF8
#include <ncbind.hpp>

#include <string>
#include <memory>

// const tjs_char*をUTF8文字列へ変換
std::string
convertTtstrToUtf8String(const tjs_char *str)
{
	std::string ret;
	int len = TVPWideCharToUtf8String(str, nullptr);
	if (len > 0) {
		auto buf = std::make_unique<char[]>(len);
		if (buf) {
			char *dat = buf.get();
			len = TVPWideCharToUtf8String(str, dat);
			ret.assign(dat, len);
		}
	}
	return std::move(ret);
}

// const char* をttstrに変換
ttstr
convertUtf8StringToTtstr(const char *str) 
{
	ttstr ret;
	tjs_uint len = TVPUtf8ToWideCharString(str, nullptr);
	if (len > 0) {
		auto buf = std::make_unique<tjs_char[]>(len);
		if (buf) {
			tjs_char *dat = buf.get();
			len = TVPUtf8ToWideCharString(str, dat);
			ret = ttstr(dat, len);
		}
	}
	return std::move(ret);
}