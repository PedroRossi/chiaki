// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifdef __SWITCH__
#include <switch.h>
#endif

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

#include <discoverymanager.h>
#include <gui.h>

#define PING_MS		500
#define HOSTS_MAX	16
#define DROP_PINGS	3

static void DiscoveryServiceHostsCallback(ChiakiDiscoveryHost *hosts, size_t hosts_count, void *user);

DiscoveryManager::DiscoveryManager(MainApplication *app, Callback cb) : app(app), log(app->GetLog()), cb(cb)
{
	service_active = false;
}

DiscoveryManager::~DiscoveryManager()
{
	if(service_active)
		chiaki_discovery_service_fini(&service);
}

void DiscoveryManager::TriggerCallback()
{
	cb(hosts);
}

void DiscoveryManager::SetActive(bool active)
{
	if(service_active == active)
		return;
	service_active = active;

	if(active)
	{
		ChiakiDiscoveryServiceOptions options;
		options.ping_ms = PING_MS;
		options.hosts_max = HOSTS_MAX;
		options.host_drop_pings = DROP_PINGS;
		options.cb = DiscoveryServiceHostsCallback;
		options.cb_user = this;

		sockaddr_in addr = {};
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = GetIPv4BroadcastAddr();
		options.send_addr = reinterpret_cast<sockaddr *>(&addr);
		options.send_addr_size = sizeof(addr);

		ChiakiErrorCode err = chiaki_discovery_service_init(&service, &options, log);
		if(err != CHIAKI_ERR_SUCCESS)
		{
			service_active = false;
			CHIAKI_LOGE(log, "DiscoveryManager failed to init Discovery Service");
			return;
		}
	}
	else
	{
		chiaki_discovery_service_fini(&service);
		hosts = {};
		TriggerCallback();
	}
}

uint32_t DiscoveryManager::GetIPv4BroadcastAddr()
{
#ifdef __SWITCH__
	// Switch will not send out a broadcast to the generic 255.255.255.255 IP.
	// It needs to be the correct broadcast address for the subnet we are in.
	uint32_t current_addr, subnet_mask;
	// init nintendo net interface service
	Result rc = nifmInitialize(NifmServiceType_User);
	if(R_SUCCEEDED(rc))
	{
		// read current IP and netmask
		rc = nifmGetCurrentIpConfigInfo(
			&current_addr, &subnet_mask,
			NULL, NULL, NULL);
		nifmExit();
	}
	else
	{
		CHIAKI_LOGE(this->log, "Failed to get nintendo nifmGetCurrentIpConfigInfo");
		return 0xffffffff;
	}
	return current_addr | (~subnet_mask);
#else
	return 0xffffffff;
#endif
}

ChiakiErrorCode DiscoveryManager::SendWakeup(const std::string &host, const char *regist_key /*[CHIAKI_SESSION_AUTH_SIZE]*/, bool ps5)
{
	char key[CHIAKI_SESSION_AUTH_SIZE + 1];
	memcpy(key, regist_key, CHIAKI_SESSION_AUTH_SIZE);
	key[CHIAKI_SESSION_AUTH_SIZE] = 0;

	char *endptr;
	uint64_t credential = strtoull(key, &endptr, 16);
	bool ok = strlen(key) <= 8 && *key && !*endptr;
	if(!ok)
	{
		CHIAKI_LOGE(log, "DiscoveryManager got invalid regist key for wakeup");
		return CHIAKI_ERR_INVALID_DATA;
	}

	ChiakiErrorCode err = chiaki_discovery_wakeup(log, service_active ? &service.discovery : nullptr, host.c_str(), credential, ps5);

	if(err != CHIAKI_ERR_SUCCESS)
		CHIAKI_LOGE(log, "DiscoveryManager failed to send packed: %s", chiaki_error_string(err));
	return err;
}

void DiscoveryManager::DiscoveryServiceHosts(const std::vector<DiscoveryHost> &hosts)
{
	this->hosts = std::move(hosts);

	for(const auto &host : this->hosts)
	{
		CHIAKI_LOGI(this->log, "--");
		CHIAKI_LOGI(this->log, "Discovered Host:");
		CHIAKI_LOGI(this->log, "State:                             %s", chiaki_discovery_host_state_string(host.state));
		CHIAKI_LOGI(this->log, "System Version:                    %s", host.system_version.c_str());
		CHIAKI_LOGI(this->log, "Device Discovery Protocol Version: %s", host.device_discovery_protocol_version.c_str());
		CHIAKI_LOGI(this->log, "Request Port:                      %hu", (unsigned short)host.host_request_port);
		CHIAKI_LOGI(this->log, "Host Addr:                         %s", host.host_addr.c_str());
		CHIAKI_LOGI(this->log, "Host Name:                         %s", host.host_name.c_str());
		CHIAKI_LOGI(this->log, "Host Type:                         %s", host.host_type.c_str());
		CHIAKI_LOGI(this->log, "Host ID:                           %s", host.host_id.c_str());
		CHIAKI_LOGI(this->log, "Running App Title ID:              %s", host.running_app_titleid.c_str());
		CHIAKI_LOGI(this->log, "Running App Name:                  %s", host.running_app_name.c_str());
		CHIAKI_LOGI(this->log, "--");
	}

	TriggerCallback();
}

class DiscoveryManagerPrivate
{
	public:
		static void DiscoveryServiceHosts(DiscoveryManager *discovery_manager, const std::vector<DiscoveryHost> &hosts)
		{
			discovery_manager->app->Post([discovery_manager, hosts]() {
				discovery_manager->DiscoveryServiceHosts(hosts);
			});
		}
};

static void DiscoveryServiceHostsCallback(ChiakiDiscoveryHost *hosts, size_t hosts_count, void *user)
{
	std::vector<DiscoveryHost> hosts_list;
	hosts_list.reserve(hosts_count);

	for(size_t i=0; i<hosts_count; i++)
	{
		ChiakiDiscoveryHost *h = hosts + i;
		DiscoveryHost o = {};
		o.ps5 = chiaki_discovery_host_is_ps5(h);
		o.state = h->state;
		o.host_request_port = o.host_request_port;
#define CONVERT_STRING(name) if(h->name) { o.name = h->name; }
		CHIAKI_DISCOVERY_HOST_STRING_FOREACH(CONVERT_STRING)
#undef CONVERT_STRING
		hosts_list.push_back(o);
	}

	DiscoveryManagerPrivate::DiscoveryServiceHosts(reinterpret_cast<DiscoveryManager *>(user), hosts_list);
}
