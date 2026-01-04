// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL
// SPDX-FileCopyrightText: 2025 Florian Märkl <info@florianmaerkl.de>

#ifndef CHIAKI_SWITCH_DISPLAY_SERVER_VIEW_H
#define CHIAKI_SWITCH_DISPLAY_SERVER_VIEW_H

#include <borealis.hpp>

#include "host.h"

class MainApplication;
class IO;

class DisplayServerView : public brls::List
{
	private:
		MainApplication *app;
		IO *io;
		DisplayServer server;
		Settings *settings;

		brls::ListItem *connect_item;
		brls::ListItem *wakeup_item;
		brls::ListItem *register_item;

		brls::Label *info_header_item;
		brls::ListItem *info_state_item;
		brls::ListItem *info_source_item;
		brls::ListItem *info_host_item;
		brls::ListItem *info_mac_item;
		brls::ListItem *info_registered_item;

		brls::Label *edit_header_item;
		brls::ListItem *edit_item;
		brls::ListItem *delete_item;

		void UpdateViewVisibility();

	public:
		DisplayServerView(MainApplication *app, const DisplayServer &server);
		~DisplayServerView();

		void SetServer(const DisplayServer &server);
		const DisplayServer &GetServer() const { return server; }

		void Register();
		void Wakeup(brls::View *view);
};

#endif
