// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_SWITCH_STREAMSESSION_H
#define CHIAKI_SWITCH_STREAMSESSION_H

#include <chiaki/session.h>
#include <chiaki/opusdecoder.h>

#include <string>
#include <functional>
#include <map>

class IO;
class Settings;

struct StreamSessionConnectInfo
{
	IO *io;
	ChiakiLog *log;
	Settings *settings;
	ChiakiTarget target;
	std::string host;
	char rp_regist_key[CHIAKI_SESSION_AUTH_SIZE];
	uint8_t rp_key[0x10];
	ChiakiConnectVideoProfile video_profile;

	StreamSessionConnectInfo(
			IO *io,
			ChiakiLog *log,
			Settings *settings,
			ChiakiTarget target,
			std::string host,
			const char *rp_regist_key /* [CHIAKI_SESSION_AUTH_SIZE] */,
			const uint8_t *rp_key /* [0x10] */);
};

class StreamSession
{
	private:
		IO *io;
		ChiakiLog *log;

		ChiakiSession session;
		ChiakiOpusDecoder opus_decoder;
		ChiakiConnectVideoProfile video_profile;

		ChiakiControllerState controller_state = {0};
		std::map<uint32_t, int8_t> finger_id_touch_id;

		std::function<void()> chiaki_event_connected_cb = nullptr;
		std::function<void(bool)> chiaki_even_login_pin_request_cb = nullptr;
		std::function<void(uint8_t, uint8_t)> chiaki_event_rumble_cb = nullptr;
		std::function<void(ChiakiQuitEvent *)> chiaki_event_quit_cb = nullptr;
		std::function<void(ChiakiControllerState *, std::map<uint32_t, int8_t> *)> io_read_controller_cb = nullptr;

	public:
		StreamSession(const StreamSessionConnectInfo &connect_info);
		~StreamSession();

		void Start();
		void Stop();

		void SendFeedbackState();

		void SetEventConnectedCallback(std::function<void()> chiaki_event_connected_cb);
		void SetEventLoginPinRequestCallback(std::function<void(bool)> chiaki_even_login_pin_request_cb);
		void SetEventRumbleCallback(std::function<void(uint8_t, uint8_t)> chiaki_event_rumble_cb);
		void SetEventQuitCallback(std::function<void(ChiakiQuitEvent *)> chiaki_event_quit_cb);
		void SetReadControllerCallback(std::function<void(ChiakiControllerState *, std::map<uint32_t, int8_t> *)> io_read_controller_cb);

		void ConnectionEventCB(ChiakiEvent *event);
};

#endif
