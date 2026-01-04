// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_DISCOVERYMANAGER_H
#define CHIAKI_DISCOVERYMANAGER_H

#include <chiaki/discoveryservice.h>
#include <chiaki/log.h>

#include "host.h"

#include <functional>

class MainApplication;

class DiscoveryManager
{
	friend class DiscoveryManagerPrivate;
	public:
		using Callback = std::function<void(const std::vector<DiscoveryHost> &hosts)>;

	private:
		MainApplication *app;
		ChiakiLog *log;
		Callback cb;
		ChiakiDiscoveryService service;
		bool service_active;
		std::vector<DiscoveryHost> hosts;

		void TriggerCallback();
		void DiscoveryServiceHosts(const std::vector<DiscoveryHost> &hosts);

		uint32_t GetIPv4BroadcastAddr();

	public:
		DiscoveryManager(MainApplication *app, Callback cb);
		~DiscoveryManager();

		void SetActive(bool active);

		ChiakiErrorCode SendWakeup(const std::string &host, const char *regist_key /*[CHIAKI_SESSION_AUTH_SIZE]*/, bool ps5);

		const std::vector<DiscoveryHost> &GetHosts() const { return hosts; }
};

#endif //CHIAKI_DISCOVERYMANAGER_H
