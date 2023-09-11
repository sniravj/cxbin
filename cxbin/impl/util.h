#ifndef STRINGUTIL_UTIL_1630743680042_H
#define STRINGUTIL_UTIL_1630743680042_H
#include <string>
#include <vector>
#include <codecvt>
#include <cstdio>

#ifdef _MSC_VER
# ifndef strcasecmp
#  define strcasecmp _stricmp
# endif
# ifndef strncasecmp
#  define strncasecmp _strnicmp
# endif
# ifndef strdup
#  define strdup _strdup
# endif
#endif

#define GET_LINE() do { if (!fgets(buf, 1024, f)) return false; } while (0)
#define GET_WORD() do { if (fscanf(f, " %1023s", buf) != 1) return false; } while (0)
#define COND_READ(cond, where, len) do { if ((cond) && !fread((void *)&(where), (len), 1, f)) return false; } while (0)
#define FPRINTF(...) do { if (fprintf(__VA_ARGS__) < 0) return false; } while (0)
#define FWRITE(ptr, size, nmemb, stream) do { if (fwrite((ptr), (size), (nmemb), (stream)) != (nmemb)) return false; } while (0)

namespace cxbin
{
	std::string toLowerCase(const std::string& str);
	/*×Ö·û´®È¥µôÊ×Î²¿Õ¶Î*/
	std::string trimHeadTail(const std::string& str);
	/*×Ö·û´®·Ö¸î*/
	std::vector<std::string> splitString(const std::string& str, const std::string& delim = ",");
	bool str_replace(std::string& strContent, const char* pszOld, const char* pszNew);

	/*»ñÈ¡ºó×º*/
	std::string extensionFromFileName(const std::string& fileName, bool lowerCase = false);
	bool begins_with(const char* s1, const char* s2);


	void skip_comments(FILE* f);
}

#define LINE_IS(text) cxbin::begins_with(buf, text)

#endif // STRINGUTIL_UTIL_1630743680042_H