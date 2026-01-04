// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <streamsession.h>
#include <io.h>
#include <settings.h>

#include <chiaki/opusdecoder.h>

StreamSessionConnectInfo::StreamSessionConnectInfo(
	IO *io,
	ChiakiLog *log,
	Settings *settings,
	ChiakiTarget target,
	std::string host,
	const char *rp_regist_key,
	const uint8_t *rp_key)
	: io(io), log(log), settings(settings), target(target), host(host)
{
	memcpy(this->rp_regist_key, rp_regist_key, sizeof(this->rp_regist_key));
	memcpy(this->rp_key, rp_key, sizeof(this->rp_key));
	chiaki_connect_video_profile_preset(&video_profile, settings->GetVideoResolution(), settings->GetVideoFPS());
}

static void InitAudioCB(unsigned int channels, unsigned int rate, void *user);
static bool VideoCB(uint8_t *buf, size_t buf_size, void *user);
static void AudioCB(int16_t *buf, size_t samples_count, void *user);
static void EventCB(ChiakiEvent *event, void *user);

StreamSession::StreamSession(const StreamSessionConnectInfo &connect_info) : io(connect_info.io), log(connect_info.log)
{
	// Build chiaki ps4 stream session
	chiaki_opus_decoder_init(&(this->opus_decoder), this->log);
	ChiakiAudioSink audio_sink;
	ChiakiConnectInfo chiaki_connect_info = {};

	chiaki_connect_info.host = connect_info.host.c_str();
	chiaki_connect_info.video_profile = connect_info.video_profile;
	chiaki_connect_info.video_profile_auto_downgrade = true;

	chiaki_connect_info.ps5 = chiaki_target_is_ps5(connect_info.target);

	memcpy(chiaki_connect_info.regist_key, connect_info.rp_regist_key, sizeof(chiaki_connect_info.regist_key));
	memcpy(chiaki_connect_info.morning, connect_info.rp_key, sizeof(chiaki_connect_info.morning));

	ChiakiErrorCode err = chiaki_session_init(&(this->session), &chiaki_connect_info, this->log);
	if(err != CHIAKI_ERR_SUCCESS)
		throw Exception(chiaki_error_string(err));
	// audio setting_cb and frame_cb
	chiaki_opus_decoder_set_cb(&this->opus_decoder, InitAudioCB, AudioCB, io);
	chiaki_opus_decoder_get_sink(&this->opus_decoder, &audio_sink);
	chiaki_session_set_audio_sink(&this->session, &audio_sink);
	chiaki_session_set_video_sample_cb(&this->session, VideoCB, io);
	chiaki_session_set_event_cb(&this->session, EventCB, this);

	// init controller states
	chiaki_controller_state_set_idle(&this->controller_state);
}

StreamSession::~StreamSession()
{
	chiaki_session_join(&session);
	chiaki_session_fini(&session);
	chiaki_opus_decoder_fini(&opus_decoder);
}

void StreamSession::Start()
{
	ChiakiErrorCode err = chiaki_session_start(&this->session);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		chiaki_session_fini(&this->session);
		throw Exception("Chiaki Session Start failed");
	}
}

void StreamSession::Stop()
{
	chiaki_session_stop(&session);
}

void StreamSession::SendFeedbackState()
{
	// send controller/joystick key
	if(this->io_read_controller_cb != nullptr)
		this->io_read_controller_cb(&this->controller_state, &finger_id_touch_id);

	chiaki_session_set_controller_state(&this->session, &this->controller_state);
}

void StreamSession::ConnectionEventCB(ChiakiEvent *event)
{
	switch(event->type)
	{
		case CHIAKI_EVENT_CONNECTED:
			CHIAKI_LOGI(this->log, "EventCB CHIAKI_EVENT_CONNECTED");
			if(this->chiaki_event_connected_cb != nullptr)
				this->chiaki_event_connected_cb();
			break;
		case CHIAKI_EVENT_LOGIN_PIN_REQUEST:
			CHIAKI_LOGI(this->log, "EventCB CHIAKI_EVENT_LOGIN_PIN_REQUEST");
			if(this->chiaki_even_login_pin_request_cb != nullptr)
				this->chiaki_even_login_pin_request_cb(event->login_pin_request.pin_incorrect);
			break;
		case CHIAKI_EVENT_RUMBLE:
			CHIAKI_LOGD(this->log, "EventCB CHIAKI_EVENT_RUMBLE");
			if(this->chiaki_event_rumble_cb != nullptr)
				this->chiaki_event_rumble_cb(event->rumble.left, event->rumble.right);
			break;
		case CHIAKI_EVENT_QUIT:
			CHIAKI_LOGI(this->log, "EventCB CHIAKI_EVENT_QUIT");
			if(this->chiaki_event_quit_cb != nullptr)
				this->chiaki_event_quit_cb(&event->quit);
			break;
		default:
			break;
	}
}

void StreamSession::SetEventConnectedCallback(std::function<void()> chiaki_event_connected_cb)
{
	this->chiaki_event_connected_cb = chiaki_event_connected_cb;
}

void StreamSession::SetEventLoginPinRequestCallback(std::function<void(bool)> chiaki_even_login_pin_request_cb)
{
	this->chiaki_even_login_pin_request_cb = chiaki_even_login_pin_request_cb;
}

void StreamSession::SetEventQuitCallback(std::function<void(ChiakiQuitEvent *)> chiaki_event_quit_cb)
{
	this->chiaki_event_quit_cb = chiaki_event_quit_cb;
}

void StreamSession::SetEventRumbleCallback(std::function<void(uint8_t, uint8_t)> chiaki_event_rumble_cb)
{
	this->chiaki_event_rumble_cb = chiaki_event_rumble_cb;
}

void StreamSession::SetReadControllerCallback(std::function<void(ChiakiControllerState *, std::map<uint32_t, int8_t> *)> io_read_controller_cb)
{
	this->io_read_controller_cb = io_read_controller_cb;
}

static void InitAudioCB(unsigned int channels, unsigned int rate, void *user)
{
	IO *io = (IO *)user;
	io->InitAudioCB(channels, rate);
}

static bool VideoCB(uint8_t *buf, size_t buf_size, void *user)
{
	IO *io = (IO *)user;
	return io->VideoCB(buf, buf_size);
}

static void AudioCB(int16_t *buf, size_t samples_count, void *user)
{
	IO *io = (IO *)user;
	io->AudioCB(buf, samples_count);
}

static void EventCB(ChiakiEvent *event, void *user)
{
	StreamSession *session = reinterpret_cast<StreamSession *>(user);
	session->ConnectionEventCB(event);
}

