// SPDX-FileCopyrightText: 2025 Florian Märkl <info@florianmaerkl.de>
// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_SWITCH_MANUAL_HOST_DIALOG_H
#define CHIAKI_SWITCH_MANUAL_HOST_DIALOG_H

#include "host.h"

#include <borealis.hpp>

struct TargetDef;
class MainApplication;

class ManualHostDialog : public brls::AppletFrame
{
	private:
		MainApplication *app;

		int host_id;

		brls::InputListItem *host_input;
		brls::SelectListItem *registered_host_select;
		std::vector<RegisteredHost> registered_hosts;

	public:
		ManualHostDialog(MainApplication *app, ManualHost host);
		~ManualHostDialog();

		void Save();

		static void Present(MainApplication *app, ManualHost host = ManualHost());
};

#endif
