// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_HOST_H
#define CHIAKI_HOST_H

#include <chiaki/controller.h>
#include <chiaki/discovery.h>
#include <chiaki/log.h>
#include <chiaki/opusdecoder.h>
#include <chiaki/regist.h>

#include <netinet/in.h>
#include <string>
#include <cstring>

class DiscoveryManager;
class ConfigParser;

enum RegistError
{
	HOST_REGISTER_OK,
	HOST_REGISTER_ERROR_SETTING_PSNACCOUNTID,
	HOST_REGISTER_ERROR_SETTING_PSNONLINEID
};

class Settings;

class HostMAC
{
	private:
		uint8_t mac[6];

	public:
		HostMAC()								{ memset(mac, 0, sizeof(mac)); }
		HostMAC(const HostMAC &o)				{ memcpy(mac, o.GetMAC(), sizeof(mac)); }
		explicit HostMAC(const uint8_t mac[6])	{ memcpy(this->mac, mac, sizeof(this->mac)); }
		const uint8_t *GetMAC() const			{ return mac; }
		uint64_t GetValue() const
		{
			return ((uint64_t)mac[0] << 0x28)
				| ((uint64_t)mac[1] << 0x20)
				| ((uint64_t)mac[2] << 0x18)
				| ((uint64_t)mac[3] << 0x10)
				| ((uint64_t)mac[4] << 0x8)
				| mac[5];
		}
		std::string ToString() const;
		void Parse(const std::string &str, bool *ok);
};

static bool operator==(const HostMAC &a, const HostMAC &b)	{ return memcmp(a.GetMAC(), b.GetMAC(), 6) == 0; }
static bool operator<(const HostMAC &a, const HostMAC &b)	{ return a.GetValue() < b.GetValue(); }

class RegisteredHost
{
	private:
		ChiakiTarget target;
		std::string ap_ssid;
		std::string ap_bssid;
		std::string ap_key;
		std::string ap_name;
		HostMAC server_mac;
		std::string server_nickname;
		char rp_regist_key[CHIAKI_SESSION_AUTH_SIZE];
		uint32_t rp_key_type;
		uint8_t rp_key[0x10];

	public:
		RegisteredHost();
		RegisteredHost(const RegisteredHost &o);

		RegisteredHost(const ChiakiRegisteredHost &chiaki_host);

		ChiakiTarget GetTarget() const					{ return target; }
		const HostMAC &GetServerMAC() const 			{ return server_mac; }
		const std::string &GetServerNickname() const	{ return server_nickname; }
		void SaveToSettings(std::ostream &s) const;
		static RegisteredHost LoadFromSettings(ChiakiLog *log, ConfigParser &p);
		const char *GetRpRegistKey() const				{ return rp_regist_key; }
		const uint8_t *GetRpKey() const					{ return rp_key; }
};

class ManualHost
{
	private:
		int id;
		std::string host;
		bool registered;
		HostMAC registered_mac;

	public:
		ManualHost();
		ManualHost(int id, const std::string &host, bool registered, const HostMAC &registered_mac);
		ManualHost(int id, const ManualHost &o);

		int GetID() const 			{ return id; }
		std::string GetHost() const	{ return host; }
		bool GetRegistered() const	{ return registered; }
		HostMAC GetMAC() const 		{ return registered_mac; }

		void Register(const RegisteredHost &registered_host) { this->registered = true; this->registered_mac = registered_host.GetServerMAC(); }

		void SaveToSettings(std::ostream &s) const;
		static ManualHost LoadFromSettings(ChiakiLog *log, ConfigParser &p);
};

struct DiscoveryHost
{
	bool ps5;
	ChiakiDiscoveryHostState state;
	uint16_t host_request_port;
#define STRING_MEMBER(name) std::string name;
	CHIAKI_DISCOVERY_HOST_STRING_FOREACH(STRING_MEMBER)
#undef STRING_MEMBER

	HostMAC GetHostMAC() const;
};

struct DisplayServerID
{
	int manual_host_id;
	std::string discovered_host_id;
	bool discovered;
};

static bool operator==(const DisplayServerID &a, const DisplayServerID &b)
{
	if(a.discovered != b.discovered)
		return false;
	if(a.discovered)
		return a.discovered_host_id == b.discovered_host_id;
	return a.manual_host_id == b.manual_host_id;
}

static bool operator<(const DisplayServerID &a, const DisplayServerID &b)
{
	if(a.discovered != b.discovered)
		return b.discovered;
	if(a.discovered)
		return a.discovered_host_id < b.discovered_host_id;
	return a.manual_host_id < b.manual_host_id;
}

struct DisplayServer
{
	DiscoveryHost discovery_host;
	ManualHost manual_host;
	bool discovered;

	RegisteredHost registered_host;
	bool registered;

	std::string GetHostAddr() const { return discovered ? discovery_host.host_addr : manual_host.GetHost(); }
	bool IsPS5() const { return discovered ? discovery_host.ps5 :
		(registered ? chiaki_target_is_ps5(registered_host.GetTarget()) : false); }

	DisplayServerID GetID() const
	{
		DisplayServerID r = {};
		r.discovered = discovered;
		if(discovered)
			r.discovered_host_id = discovery_host.host_id;
		else
			r.manual_host_id = manual_host.GetID();
		return r;
	}

	std::string GetDisplayName() const
	{
		if(discovered)
			return discovery_host.host_name;
		return manual_host.GetHost();
	}

	ChiakiErrorCode Wakeup() const;
};

#endif
