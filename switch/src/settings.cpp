// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <settings.h>
#include <configparser.h>
#include <host.h>

#include <chiaki/base64.h>

#include <fstream>

Settings::Settings()
{
#if defined(__SWITCH__)
	chiaki_log_init(&this->log, CHIAKI_LOG_ALL & ~(CHIAKI_LOG_VERBOSE | CHIAKI_LOG_DEBUG), chiaki_log_cb_print, NULL);
#else
	chiaki_log_init(&this->log, CHIAKI_LOG_ALL & ~CHIAKI_LOG_VERBOSE, chiaki_log_cb_print, NULL);
#endif
}

Settings *Settings::instance = nullptr;

Settings *Settings::GetInstance()
{
	if(instance == nullptr)
	{
		instance = new Settings;
		instance->ParseFile();
	}
	return instance;
}

ChiakiLog *Settings::GetLogger()
{
	return &this->log;
}

void Settings::AddRegisteredHost(const RegisteredHost &host)
{
	registered_hosts[host.GetServerMAC()] = host;
}

void Settings::AddManualHost(const ManualHost &host)
{
	ManualHost h = host;
	if(h.GetID() < 0)
		h = ManualHost(manual_hosts_id_next++, h.GetHost(), h.GetRegistered(), h.GetMAC());
	manual_hosts[h.GetID()] = h;
	WriteFile();
}

void Settings::DeleteManualHost(int id)
{
	manual_hosts.erase(id);
	WriteFile();
}

void Settings::ParseFile()
{
	CHIAKI_LOGI(&this->log, "Parse config file %s", this->filename);

	ConfigParser p(this->filename);
	if(!p.IsOpen())
	{
		CHIAKI_LOGW(&this->log, "Failed to open config file");
		return;
	}
	p.Next();
	while(p.Cur().kind != ConfigEntry::Kind::Eof)
	{
		const auto &entry = p.Cur();
		switch(entry.kind)
		{
			case ConfigEntry::Kind::Section:
				if(entry.key == "registered_host")
				{
					auto h = RegisteredHost::LoadFromSettings(&log, p);
					AddRegisteredHost(h);
					continue; // RegisteredHost::LoadFromSettings already called p.Next()
				}
				else if(entry.key == "manual_host")
				{
					auto h = ManualHost::LoadFromSettings(&log, p);
					AddManualHost(h);
					continue; // ManualHost::LoadFromSettings already called p.Next()
				}
				else
					CHIAKI_LOGW(&log, "Unknown section in config file: \"%s\"", entry.key.c_str());
				break;
			case ConfigEntry::Kind::Value:
				if(entry.key == "video_resolution")
					SetVideoResolution(entry.value);
				else if(entry.key == "video_fps")
					SetVideoFPS(entry.value);
				else if(entry.key == "psn_account_id")
					SetPSNAccountID(entry.value);
				else
					CHIAKI_LOGW(&log, "Unknown key in config file: \"%s\"", entry.key.c_str());
			case ConfigEntry::Kind::Invalid:
				CHIAKI_LOGW(&log, "Invalid line in config file: %s", entry.key.c_str());
			default:
				break;
		}
		p.Next();
	}
}

void Settings::WriteFile()
{
	std::fstream config_file;
	CHIAKI_LOGI(&this->log, "Write config file %s", this->filename);
	// flush file (trunc)
	// the config file is completely overwritten
	config_file.open(this->filename, std::fstream::out | std::ofstream::trunc);
	std::string line;
	std::string value;

	if(!config_file.is_open())
	{
		CHIAKI_LOGE(&this->log, "Failed to open config file");
		return;
	}

	// save global settings
	CHIAKI_LOGD(&this->log, "Write Global config file %s", this->filename);

	if(this->global_video_resolution)
		config_file << "video_resolution = \""
					<< this->ResolutionPresetToString(this->GetVideoResolution())
					<< "\"\n";

	if(this->global_video_fps)
		config_file << "video_fps = "
					<< this->FPSPresetToString(this->GetVideoFPS())
					<< "\n";

	if(this->global_psn_online_id.length())
		config_file << "psn_online_id = \"" << this->global_psn_online_id << "\"\n";

	if(this->global_psn_account_id.length())
		config_file << "psn_account_id = \"" << this->global_psn_account_id << "\"\n";

	for(auto entry : registered_hosts)
	{
		config_file << "\n[registered_host]\n";
		entry.second.SaveToSettings(config_file);
	}

	for(auto entry: manual_hosts)
	{
		config_file << "\n[manual_host]\n";
		entry.second.SaveToSettings(config_file);
	}

	config_file.close();
}

std::string Settings::ResolutionPresetToString(ChiakiVideoResolutionPreset resolution)
{
	switch(resolution)
	{
		case CHIAKI_VIDEO_RESOLUTION_PRESET_360p:
			return "360p";
		case CHIAKI_VIDEO_RESOLUTION_PRESET_540p:
			return "540p";
		case CHIAKI_VIDEO_RESOLUTION_PRESET_720p:
			return "720p";
		case CHIAKI_VIDEO_RESOLUTION_PRESET_1080p:
			return "1080p";
	}
	return "UNKNOWN";
}

int Settings::ResolutionPresetToInt(ChiakiVideoResolutionPreset resolution)
{
	switch(resolution)
	{
		case CHIAKI_VIDEO_RESOLUTION_PRESET_360p:
			return 360;
		case CHIAKI_VIDEO_RESOLUTION_PRESET_540p:
			return 540;
		case CHIAKI_VIDEO_RESOLUTION_PRESET_720p:
			return 720;
		case CHIAKI_VIDEO_RESOLUTION_PRESET_1080p:
			return 1080;
	}
	return 0;
}

ChiakiVideoResolutionPreset Settings::StringToResolutionPreset(std::string value)
{
	if(value.compare("1080p") == 0)
		return CHIAKI_VIDEO_RESOLUTION_PRESET_1080p;
	else if(value.compare("720p") == 0)
		return CHIAKI_VIDEO_RESOLUTION_PRESET_720p;
	else if(value.compare("540p") == 0)
		return CHIAKI_VIDEO_RESOLUTION_PRESET_540p;
	else if(value.compare("360p") == 0)
		return CHIAKI_VIDEO_RESOLUTION_PRESET_360p;

	// default
	CHIAKI_LOGE(&this->log, "Unable to parse String resolution: %s",
		value.c_str());

	return CHIAKI_VIDEO_RESOLUTION_PRESET_720p;
}

std::string Settings::FPSPresetToString(ChiakiVideoFPSPreset fps)
{
	switch(fps)
	{
		case CHIAKI_VIDEO_FPS_PRESET_30:
			return "30";
		case CHIAKI_VIDEO_FPS_PRESET_60:
			return "60";
	}
	return "UNKNOWN";
}

int Settings::FPSPresetToInt(ChiakiVideoFPSPreset fps)
{
	switch(fps)
	{
		case CHIAKI_VIDEO_FPS_PRESET_30:
			return 30;
		case CHIAKI_VIDEO_FPS_PRESET_60:
			return 60;
	}
	return 0;
}

ChiakiVideoFPSPreset Settings::StringToFPSPreset(std::string value)
{
	if(value.compare("60") == 0)
		return CHIAKI_VIDEO_FPS_PRESET_60;
	else if(value.compare("30") == 0)
		return CHIAKI_VIDEO_FPS_PRESET_30;

	// default
	CHIAKI_LOGE(&this->log, "Unable to parse String fps: %s",
		value.c_str());

	return CHIAKI_VIDEO_FPS_PRESET_30;
}

std::string Settings::GetPSNOnlineID()
{
	return global_psn_online_id;
}

void Settings::SetPSNOnlineID(std::string psn_online_id)
{
	global_psn_online_id = psn_online_id;
}

std::string Settings::GetPSNAccountID()
{
	return global_psn_account_id;
}

void Settings::SetPSNAccountID(std::string psn_account_id)
{
	global_psn_account_id = psn_account_id;
}

ChiakiVideoResolutionPreset Settings::GetVideoResolution()
{
	return global_video_resolution;
}

void Settings::SetVideoResolution(ChiakiVideoResolutionPreset value)
{
	global_video_resolution = value;
}

void Settings::SetVideoResolution(std::string value)
{
	ChiakiVideoResolutionPreset p = StringToResolutionPreset(value);
	this->SetVideoResolution(p);
}

ChiakiVideoFPSPreset Settings::GetVideoFPS()
{
	return global_video_fps;
}

void Settings::SetVideoFPS(ChiakiVideoFPSPreset value)
{
	global_video_fps = value;
}

void Settings::SetVideoFPS(std::string value)
{
	ChiakiVideoFPSPreset p = StringToFPSPreset(value);
	this->SetVideoFPS(p);
}

#ifdef CHIAKI_ENABLE_SWITCH_OVERCLOCK
int Settings::GetCPUOverclock(Host *host)
{
	if(host == nullptr)
		return this->global_cpu_overclock;
	else
		return host->cpu_overclock;
}

void Settings::SetCPUOverclock(Host *host, int value)
{
	int oc = OC_1326;
	if(value > OC_1580)
		// max OC
		oc = OC_1785;
	else if(OC_1580 >= value && value > OC_1326)
		oc = OC_1580;
	else if(OC_1326 >= value && value > OC_1220)
		oc = OC_1326;
	else if(OC_1220 >= value && value > OC_1020)
		oc = OC_1220;
	else if(OC_1020 >= value)
		// no overclock
		// default nintendo switch value
		oc = OC_1020;
	if(host == nullptr)
		this->global_cpu_overclock = oc;
	else
		host->cpu_overclock = oc;
}

void Settings::SetCPUOverclock(Host *host, std::string value)
{
	int v = atoi(value.c_str());
	this->SetCPUOverclock(host, v);
}
#endif
