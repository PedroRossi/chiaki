// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_GUI_H
#define CHIAKI_GUI_H

#include <glad.h>
#include <GLFW/glfw3.h>
#include "nanovg.h"
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"

#include <borealis.hpp>

#include <map>

#include "discoverymanager.h"
#include "host.h"
#include "io.h"
#include "settings.h"

class DisplayServerView;
class StreamSession;
class PSRemotePlay;

class MainApplication
{
	private:
		Settings *settings;
		ChiakiLog *log;
		DiscoveryManager *discovery_manager;
		IO *io;

		StreamSession *session;
		PSRemotePlay *stream_view;

		brls::TabFrame *tab_frame;
		brls::SidebarItem *config_sidebar_item;
		std::list<std::function<void()>> queue;

		std::vector<DisplayServer> display_servers;
		std::map<DisplayServerID, DisplayServerView *> server_views;
		std::vector<DisplayServerView *> server_views_ordered;

		bool BuildConfigurationMenu(brls::List *ls);
		void UpdateDisplayServers();
		void UpdateDisplayServerViews();

		void Disconnect();

	public:
		MainApplication();
		~MainApplication();

		bool Run();
		void Post(std::function<void()> f);
		ChiakiLog *GetLog() { return log; }
		Settings *GetSettings() { return settings; }
		DiscoveryManager *GetDiscoveryManager() { return discovery_manager; }

		void HostsUpdated();

		void Connect(const DisplayServer &server);
};

class PSRemotePlay : public brls::View
{
	private:
		brls::AppletFrame *frame;
		// to display stream on screen
		IO *io;
		// to send gamepad inputs
		StreamSession *session;
		brls::Label *label;
	public:
		PSRemotePlay(StreamSession *session);
		~PSRemotePlay();

		void SessionClosed();

		void draw(NVGcontext *vg, int x, int y, unsigned width, unsigned height, brls::Style *style, brls::FrameContext *ctx) override;
};

#endif // CHIAKI_GUI_H
