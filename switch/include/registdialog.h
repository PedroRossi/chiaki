// SPDX-FileCopyrightText: 2025 Florian Märkl <info@florianmaerkl.de>
// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_SWITCH_REGIST_DIALOG_H
#define CHIAKI_SWITCH_REGIST_DIALOG_H

#include <glad.h>
#include <GLFW/glfw3.h>

#include <borealis.hpp>

#include <chiaki/common.h>
#include <chiaki/regist.h>

struct TargetDef;
class MainApplication;

class RegistDialog : public brls::AppletFrame
{
	private:
		MainApplication *app;
		int manual_host_id;

		brls::SelectListItem *target_select;
		brls::InputListItem *host_input;
		brls::ToggleListItem *broadcast_toggle;
		brls::InputListItem *online_id_input;
		brls::InputListItem *account_id_input;
		brls::InputListItem *pin_input;

		const TargetDef *GetSelectedTarget();
		void SelectedTargetChanged();

	public:
		RegistDialog(MainApplication *app, const std::string &host = "", int manual_host_id = -1);
		~RegistDialog();

		void Register();

		static void Present(MainApplication *app, const std::string &host = "", int manual_host_id = -1);
};

class RegistExecuteDialog : public brls::AppletFrame
{
	friend class RegistExecuteDialogPrivate;

	private:
		MainApplication *app;
		std::shared_ptr<bool> alive;
		int manual_host_id;

		ChiakiLog log;
		ChiakiRegist regist;

		brls::Label *log_label;
		std::string log_buf;

	public:
		RegistExecuteDialog(MainApplication *app, const ChiakiRegistInfo &regist_info, int manual_host_id);
		~RegistExecuteDialog();

		static void Present(MainApplication *app, const ChiakiRegistInfo &regist_info, int manual_host_id);
};

#endif
