// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_SETTINGS_H
#define CHIAKI_SETTINGS_H

#include <map>

#include <chiaki/log.h>
#include "host.h"

// mutual host and settings
class Host;

class Settings
{
	protected:
		// keep constructor private (sigleton class)
		Settings();
		static Settings * instance;

	private:
		const char *filename = "chiaki.conf";
		ChiakiLog log;
		std::map<HostMAC, RegisteredHost> registered_hosts;
		std::map<int, ManualHost> manual_hosts;
		int manual_hosts_id_next = 0;

		// global_settings from psedo INI file
		ChiakiVideoResolutionPreset global_video_resolution = CHIAKI_VIDEO_RESOLUTION_PRESET_720p;
		ChiakiVideoFPSPreset global_video_fps = CHIAKI_VIDEO_FPS_PRESET_60;
		std::string global_psn_online_id = "";
		std::string global_psn_account_id = "";

	public:
		// singleton configuration
		Settings(const Settings&) = delete;
		void operator=(const Settings&) = delete;
		static Settings * GetInstance();

		ChiakiLog * GetLogger();
		void AddRegisteredHost(const RegisteredHost &host);
		void AddManualHost(const ManualHost &host);

		void DeleteManualHost(int id);

		const std::map<HostMAC, RegisteredHost> &GetRegisteredHosts() const	{ return registered_hosts; }
		const std::map<int, ManualHost> &GetManualHosts() const				{ return manual_hosts; }
		bool GetRegisteredHostRegistered(const HostMAC &mac) const	{ return registered_hosts.find(mac) != registered_hosts.end(); }
		RegisteredHost GetRegisteredHost(const HostMAC &mac) const	{ return registered_hosts.at(mac); }

		void ParseFile();
		void WriteFile();

		std::string ResolutionPresetToString(ChiakiVideoResolutionPreset resolution);
		int ResolutionPresetToInt(ChiakiVideoResolutionPreset resolution);
		ChiakiVideoResolutionPreset StringToResolutionPreset(std::string value);

		std::string FPSPresetToString(ChiakiVideoFPSPreset fps);
		int FPSPresetToInt(ChiakiVideoFPSPreset fps);
		ChiakiVideoFPSPreset StringToFPSPreset(std::string value);

		std::string GetPSNOnlineID();
		void SetPSNOnlineID(std::string psn_online_id);

		std::string GetPSNAccountID();
		void SetPSNAccountID(std::string psn_account_id);

		ChiakiVideoResolutionPreset GetVideoResolution();
		void SetVideoResolution(ChiakiVideoResolutionPreset value);
		void SetVideoResolution(std::string value);

		ChiakiVideoFPSPreset GetVideoFPS();
		void SetVideoFPS(ChiakiVideoFPSPreset value);
		void SetVideoFPS(std::string value);
};

#endif // CHIAKI_SETTINGS_H
