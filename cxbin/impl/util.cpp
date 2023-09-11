#include "util.h"
#include <cctype>
#include <string.h>
#include <locale>
#include <codecvt>

namespace cxbin
{
	std::string _changeCase(const std::string& value, bool lowerCase)
	{
		size_t len = value.length();
		std::string newvalue(value);
		for (std::string::size_type i = 0, l = newvalue.length(); i < l; ++i)
			newvalue[i] = lowerCase ? tolower(newvalue[i]) : toupper(newvalue[i]);
		return newvalue;
	}

	std::string toLowerCase(const std::string& str)
	{
		return _changeCase(str,true);
	}

	bool str_replace(std::string& strContent, const char* pszOld, const char* pszNew)
	{
		std::string  strTemp;
		std::string::size_type nPos = 0;
		while (true)
		{
			nPos = strContent.find(pszOld, nPos);
			strTemp = strContent.substr(nPos + strlen(pszOld), strContent.length());
			if (nPos == std::string::npos)
			{
				break;
			}
			strContent.replace(nPos, strContent.length(), pszNew);
			strContent.append(strTemp);
			nPos += strlen(pszNew) - strlen(pszOld) + 1;
		}
		return true;
	}

	std::string trimHeadTail(const std::string& s)
	{
		std::string str = s;
		size_t n = str.find_last_not_of(" \r\n\t");
		if (n != std::string::npos) {
			str.erase(n + 1, str.size() - n);
		}
		n = str.find_first_not_of(" \r\n\t");
		if (n != std::string::npos) {
			str.erase(0, n);
		}
		return str;
	}

	std::vector<std::string> splitString(const std::string& str, const std::string& delim)
	{
		std::vector<std::string> elems;
		size_t pos = 0;
		size_t len = str.length();
		size_t delim_len = delim.length();
		if (delim_len == 0)
			return elems;
		while (pos < len)
		{
			int find_pos = str.find(delim, pos);
			if (find_pos < 0)
			{
				std::string t = str.substr(pos, len - pos);
				if (!t.empty())
					elems.push_back(t);
				break;
			}

			std::string t = str.substr(pos, find_pos - pos);
			if (!t.empty())
				elems.push_back(t);
			pos = find_pos + delim_len;
		}
		return elems;
	}

	std::string wstring2string(const std::wstring& wstr)
	{
		using convert_typeX = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_typeX, wchar_t> converterX;

		return converterX.to_bytes(wstr);
	}

	std::string wchar2char(const wchar_t* wp)
	{
		std::wstring tmpRuleStr = std::wstring(wp);
		const std::string str = wstring2string(tmpRuleStr);

		return str;
	}

	std::string extensionFromFileName(const std::string& fileName, bool lowerCase)
	{
		std::string fileType = fileName;
		int pos = fileName.find_last_of(".");
		if (pos > 0 && pos < fileType.length() - 1)
		{
			fileType = fileType.substr(pos + 1, fileType.length() - pos);
		}
		if (lowerCase)
		{
			fileType = toLowerCase(fileType);
		}
		return fileType;
	}

	bool begins_with(const char* s1, const char* s2)
	{
		using namespace ::std;
		return !strncasecmp(s1, s2, strlen(s2));
	}

	// Skip comments in an ASCII file (lines beginning with #)
	void skip_comments(FILE* f)
	{
		int c;
		bool in_comment = false;
		while (1) {
			c = fgetc(f);
			if (c == EOF)
				return;
			if (in_comment) {
				if (c == '\n')
					in_comment = false;
			}
			else if (c == '#') {
				in_comment = true;
			}
			else if (!isspace(c)) {
				break;
			}
		}
		ungetc(c, f);
	}
}