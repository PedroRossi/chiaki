// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <chiaki/base64.h>

#include <host.h>
#include <base64.h>
#include <configparser.h>

#include <cstring>
#include <regex>

std::string HostMAC::ToString() const
{
	std::string r;
	r.reserve(12);
	for(int i = 0; i < sizeof(mac); i++)
	{
		char buf[3];
		if(snprintf(buf, sizeof(buf), "%02x", mac[i]) != 2)
			return "";
		r.append(buf);
		if(i < sizeof(mac) - 1)
			r.append("-");
	}
	return r;
}

void HostMAC::Parse(const std::string &str, bool *ok)
{
	std::regex re(
		"([0-9A-Fa-f][0-9A-Fa-f])[-:]?([0-9A-Fa-f][0-9A-Fa-f])[-:]?([0-9A-Fa-f][0-9A-Fa-f])[-:]?"
		"([0-9A-Fa-f][0-9A-Fa-f])[-:]?([0-9A-Fa-f][0-9A-Fa-f])[-:]?([0-9A-Fa-f][0-9A-Fa-f])");
	std::smatch m;
	if(!std::regex_match(str, m, re))
	{
		if(ok)
			*ok = false;
		return;
	}
	for(int i = 0; i < sizeof(mac); i++)
		mac[i] = (uint8_t)strtoul(m.str(i + 1).c_str(), 0, 16);
	if(ok)
		*ok = true;
}

RegisteredHost::RegisteredHost()
{
	memset(rp_regist_key, 0, sizeof(rp_regist_key));
	memset(rp_key, 0, sizeof(rp_key));
}

RegisteredHost::RegisteredHost(const RegisteredHost &o)
	: target(o.target),
	ap_ssid(o.ap_ssid),
	ap_bssid(o.ap_bssid),
	ap_key(o.ap_key),
	ap_name(o.ap_name),
	server_mac(o.server_mac),
	server_nickname(o.server_nickname),
	rp_key_type(o.rp_key_type)
{
	memcpy(rp_regist_key, o.rp_regist_key, sizeof(rp_regist_key));
	memcpy(rp_key, o.rp_key, sizeof(rp_key));
}

RegisteredHost::RegisteredHost(const ChiakiRegisteredHost &chiaki_host)
	: server_mac(chiaki_host.server_mac)
{
	target = chiaki_host.target;
	ap_ssid = chiaki_host.ap_ssid;
	ap_bssid = chiaki_host.ap_bssid;
	ap_key = chiaki_host.ap_key;
	ap_name = chiaki_host.ap_name;
	server_nickname = chiaki_host.server_nickname;
	memcpy(rp_regist_key, chiaki_host.rp_regist_key, sizeof(rp_regist_key));
	rp_key_type = chiaki_host.rp_key_type;
	memcpy(rp_key, chiaki_host.rp_key, sizeof(rp_key));
}

void RegisteredHost::SaveToSettings(std::ostream &s) const
{
	s << "target = \"" << (int)target << "\"\n";
	s << "ap_ssid = \"" << ap_ssid << "\"\n";
	s << "ap_bssid = \"" << ap_bssid << "\"\n";
	s << "ap_key = \"" << ap_key << "\"\n";
	s << "ap_name = \"" << ap_name << "\"\n";
	s << "server_nickname = \"" << server_nickname << "\"\n";
	s << "server_mac = \"" << server_mac.ToString() << "\"\n";
	s << "rp_regist_key = \"" << Base64Encode(reinterpret_cast<const uint8_t *>(rp_regist_key), sizeof(rp_regist_key)) << "\"\n";
	s << "rp_key_type = \"" << rp_key_type << "\"\n";
	s << "rp_key = \"" << Base64Encode(rp_key, sizeof(rp_key)) << "\"\n";
}

RegisteredHost RegisteredHost::LoadFromSettings(ChiakiLog *log, ConfigParser &p)
{
	RegisteredHost r;
	while(p.Next().kind == ConfigEntry::Kind::Value)
	{
		const auto &entry = p.Cur();
		if(entry.key == "target")
		{
			try
			{
				r.target = static_cast<ChiakiTarget>(std::stoi(entry.value));
			}
			catch(std::invalid_argument &e)
			{
				CHIAKI_LOGE(log, "Invalid target value in registered_host %s", entry.value.c_str());
			}
		}
		else if(entry.key == "ap_ssid")
			r.ap_ssid = entry.value;
		else if(entry.key == "ap_bssid")
			r.ap_bssid = entry.value;
		else if(entry.key == "ap_key")
			r.ap_key = entry.value;
		else if(entry.key == "ap_name")
			r.ap_name = entry.value;
		else if(entry.key == "server_nickname")
			r.server_nickname = entry.value;
		else if(entry.key == "server_mac")
			r.server_mac.Parse(entry.value, nullptr);
		else if(entry.key == "rp_regist_key")
		{
			size_t out_sz = sizeof(r.rp_regist_key);
			chiaki_base64_decode(entry.value.c_str(), entry.value.length(), reinterpret_cast<uint8_t *>(r.rp_regist_key), &out_sz);
		}
		else if(entry.key == "rp_key")
		{
			size_t out_sz = sizeof(r.rp_key);
			chiaki_base64_decode(entry.value.c_str(), entry.value.length(), r.rp_key, &out_sz);
		}
		else
			CHIAKI_LOGE(log, "Invalid key in registered_host %s", entry.key.c_str());
	}
	return r;
}

ManualHost::ManualHost()
{
	id = -1;
	registered = false;
}

ManualHost::ManualHost(int id, const std::string &host, bool registered, const HostMAC &registered_mac)
	: id(id),
	host(host),
	registered(registered),
	registered_mac(registered_mac)
{
}

ManualHost::ManualHost(int id, const ManualHost &o)
	: id(id),
	host(o.host),
	registered(o.registered),
	registered_mac(o.registered_mac)
{
}

void ManualHost::SaveToSettings(std::ostream &s) const
{
	s << "host = \"" << host << "\"\n";
	if(registered)
		s << "registered_mac = \"" << registered_mac.ToString() << "\"\n";
}

ManualHost ManualHost::LoadFromSettings(ChiakiLog *log, ConfigParser &p)
{
	ManualHost r;
	while(p.Next().kind == ConfigEntry::Kind::Value)
	{
		const auto &entry = p.Cur();
		if(entry.key == "host")
			r.host = entry.value;
		else if(entry.key == "registered_mac")
		{
			r.registered_mac.Parse(entry.value, nullptr);
			r.registered = true;
		}
		else
			CHIAKI_LOGE(log, "Invalid key in registered_host %s", entry.key.c_str());
	}
	return r;
}

HostMAC DiscoveryHost::GetHostMAC() const
{
	HostMAC r;
	r.Parse(host_id, nullptr);
	return r;
}
