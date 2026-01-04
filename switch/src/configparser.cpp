// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <configparser.h>

#include <string>
#include <regex>

static std::regex re_empty("\\s*");
static std::regex re_section("\\[\\s*(.+)\\s*\\]");

// Known limitations: no escaping, no check for balanced quotes
static std::regex re_value("^\\s*([^=\\s]+)\\s*=\\s*\"?([^\"]*)\"?");

ConfigParser::ConfigParser(const char *file)
	: f(file), cur(ConfigEntry::Kind::Begin)
{
}

const ConfigEntry &ConfigParser::Next()
{
	std::string line;
	do
	{
		if(!std::getline(f, line))
			return cur = ConfigEntry(ConfigEntry::Kind::Eof);
	} while(std::regex_match(line, re_empty));
	std::smatch m;
	if(std::regex_match(line, m, re_section))
		return cur = ConfigEntry(ConfigEntry::Kind::Section, m[1]);
	if(std::regex_match(line, m, re_value))
		return cur = ConfigEntry(ConfigEntry::Kind::Value, m[1], m[2]);
	return cur = ConfigEntry(ConfigEntry::Kind::Invalid, line);
}

