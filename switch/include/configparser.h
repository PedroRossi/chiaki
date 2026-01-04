// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_CONFIG_PARSER
#define CHIAKI_CONFIG_PARSER

#include <fstream>

struct ConfigEntry
{
	enum class Kind {
		Begin,
		Section,
		Value,
		Invalid,
		Eof
	};

	Kind kind;
	std::string key;
	std::string value;

	ConfigEntry(Kind kind, std::string key = "", std::string value = "")
		: kind(kind), key(key), value(value) {}
};

class ConfigParser
{
	private:
		std::ifstream f;
		ConfigEntry cur;

	public:
		ConfigParser(const char *file);

		bool IsOpen()				{ return f.is_open(); }
		const ConfigEntry &Cur()	{ return cur; }
		const ConfigEntry &Next();
};

#endif
