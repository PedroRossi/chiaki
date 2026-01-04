// SPDX-FileCopyrightText: 2025 Florian Märkl <info@florianmaerkl.de>
// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <registdialog.h>
#include <gui.h>

#include <chiaki/base64.h>

#include <cstdlib>
#include <memory>

struct TargetDef
{
	ChiakiTarget target;
	const char *label;
	bool need_account_id;
};

static const TargetDef targets[] = {
	{ CHIAKI_TARGET_PS5_1, "PS5", true },
	{ CHIAKI_TARGET_PS4_10, "PS4 Firmware >= 8.0", true },
	{ CHIAKI_TARGET_PS4_9, "PS4 Firmware >= 7.0, < 8.0", true },
	{ CHIAKI_TARGET_PS4_8, "PS4 Firmware < 7.0", false }
};

#define TARGETS_COUNT (sizeof(targets) / sizeof(*targets))

#define PIN_LENGTH 8

RegistDialog::RegistDialog(MainApplication *app, const std::string &host, int manual_host_id)
	: brls::AppletFrame(true, true), app(app), manual_host_id(manual_host_id)
{
	auto list = new brls::List();
	setContentView(list);

	host_input = new brls::InputListItem("Host",
		host.empty() ? "255.255.255" : host,
		"IP Address to send the registration request to");
	list->addView(host_input);

	broadcast_toggle = new brls::ToggleListItem("Broadcast", host.empty());
	list->addView(broadcast_toggle);

	std::vector<std::string> target_labels;
	for(size_t i = 0; i < TARGETS_COUNT; i++)
		target_labels.push_back(targets[i].label);
	target_select = new brls::SelectListItem("Console", target_labels, 0);
	list->addView(target_select);
	target_select->getValueSelectedEvent()->subscribe([this](int val) {
		SelectedTargetChanged();
	});

	online_id_input = new brls::InputListItem("PSN Online-ID", "", "", "User name, case sensitive");
	list->addView(online_id_input);

	account_id_input = new brls::InputListItem("PSN Account-ID", app->GetSettings()->GetPSNAccountID(), "", "Unique user id, base64-encoded");
	list->addView(account_id_input);

	pin_input = new brls::InputListItem("PIN", "", "", std::to_string(PIN_LENGTH) + "-digit code, as displayed on the console", 8,
		brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE, true);
	list->addView(pin_input);

	auto register_item = new brls::ListItem("Start registration");
	register_item->getClickEvent()->subscribe([this](brls::View *view) {
		Register();
	});
	list->addView(register_item);

	SelectedTargetChanged();
}

RegistDialog::~RegistDialog()
{
}

const TargetDef *RegistDialog::GetSelectedTarget()
{
	unsigned int idx = target_select->getSelectedValue();
	return idx < TARGETS_COUNT ? &targets[idx] : nullptr;
}

void RegistDialog::SelectedTargetChanged()
{
	const auto *target = GetSelectedTarget();
	if(!target)
		return;
	if(target->need_account_id)
	{
		online_id_input->collapse(false);
		account_id_input->expand(false);
	}
	else
	{
		account_id_input->collapse(false);
		online_id_input->expand(false);
	}
}

void RegistDialog::Register()
{
	auto target = GetSelectedTarget();
	if(!target)
		return;
	auto host = host_input->getValue();
	ChiakiRegistInfo info = {};
	info.target = target->target;
	info.broadcast = broadcast_toggle->getToggleState();
	info.host = host.c_str();

	auto pin_str = pin_input->getValue();
	if(pin_str.length() != PIN_LENGTH || pin_str.find_first_not_of("0123456789") != std::string::npos)
	{
		brls::Application::notify("Invalid PIN: It must be exactly " + std::to_string(PIN_LENGTH) + " numeric digits.");
		return;
	}
	info.pin = std::strtoul(pin_str.c_str(), NULL, 0);

	std::string online_id_tmp;
	if(target->need_account_id)
	{
		std::string account_id_b64 = account_id_input->getValue();
		size_t account_id_size = CHIAKI_PSN_ACCOUNT_ID_SIZE;
		auto err = chiaki_base64_decode(account_id_b64.c_str(), account_id_b64.length(), info.psn_account_id, &account_id_size);
		if(err != CHIAKI_ERR_SUCCESS || account_id_size != CHIAKI_PSN_ACCOUNT_ID_SIZE)
		{
			brls::Application::notify("Invalid PSN Account-ID: It must be exactly " + std::to_string(CHIAKI_PSN_ACCOUNT_ID_SIZE) + " bytes encoded as base64.");
			return;
		}
		app->GetSettings()->SetPSNAccountID(account_id_b64);
		app->GetSettings()->WriteFile();
	}
	else
	{
		online_id_tmp = online_id_input->getValue();
		info.psn_online_id = online_id_tmp.c_str();
	}

	RegistExecuteDialog::Present(app, info, manual_host_id);
}

void RegistDialog::Present(MainApplication *app, const std::string &host, int manual_host_id)
{
	auto * const register_gui = new RegistDialog(app, host, manual_host_id);
	brls::PopupFrame::open("Register new console", register_gui, "", "Obtain credentials from a new console by PIN for streaming.");
}

static void RegistExecuteDialogLogCb(ChiakiLogLevel level, const char *msg, void *user);
static void RegistExecuteDialogRegistCb(ChiakiRegistEvent *event, void *user);

RegistExecuteDialog::RegistExecuteDialog(MainApplication *app, const ChiakiRegistInfo &regist_info, int manual_host_id)
	: brls::AppletFrame(true, true),
	app(app),
	alive(std::make_shared<bool>(true)),
	manual_host_id(manual_host_id)
{
	auto scroll_view = new brls::ScrollView(true);
	setContentView(scroll_view);

	auto layout = new brls::BoxLayout(brls::BoxLayoutOrientation::VERTICAL);
	layout->setMargins(8, 0, 8, 0);
	scroll_view->setContentView(layout);

	log_label = new brls::Label(brls::LabelStyle::SMALL, log_buf, true);
	layout->addView(log_label, true);
	layout->setResize(true);

	registerAction("Cancel", brls::Key::B, [this] { return this->onCancel(); });

	chiaki_log_init(&log, CHIAKI_LOG_ALL & (~CHIAKI_LOG_VERBOSE), RegistExecuteDialogLogCb, this);
	chiaki_regist_start(&regist, &log, &regist_info, RegistExecuteDialogRegistCb, this);
}

RegistExecuteDialog::~RegistExecuteDialog()
{
	chiaki_regist_stop(&regist);
	chiaki_regist_fini(&regist);
}

void RegistExecuteDialog::Present(MainApplication *app, const ChiakiRegistInfo &regist_info, int manual_host_id)
{
	auto * const dialog = new RegistExecuteDialog(app, regist_info, manual_host_id);
	brls::PopupFrame *popup = new brls::PopupFrame("Register new console", dialog);
	dialog->setSubtitle("Running...", "");
	brls::Application::pushView(popup);
}

class RegistExecuteDialogPrivate
{
	public:
		static void Log(RegistExecuteDialog *dialog, ChiakiLogLevel level, std::string msg)
		{
			std::weak_ptr<bool> alive = dialog->alive;
			dialog->app->Post([dialog, msg, level, alive]() {
				if(alive.expired())
					return;
				dialog->log_buf += "[" + std::string(1, chiaki_log_level_char(level)) + "] " + msg + "\n";
				dialog->log_label->setText(dialog->log_buf);
			});
		}

		static void RegistEvent(RegistExecuteDialog *dialog, ChiakiRegistEvent *event)
		{
			std::weak_ptr<bool> alive = dialog->alive;
			switch(event->type)
			{
				case CHIAKI_REGIST_EVENT_TYPE_FINISHED_SUCCESS: {
					RegisteredHost host(*event->registered_host);
					dialog->app->Post([dialog, alive, host]() {
						if(alive.expired())
							return;

						dialog->setSubtitle("Successful.", "");
						auto close_cb = [dialog] {
							brls::Application::popView(brls::ViewAnimation::FADE, []() {
								brls::Application::popView();
							});
							return true;
						};
						dialog->registerAction("Close", brls::Key::B, close_cb);
						dialog->registerAction("OK", brls::Key::A, close_cb);
						brls::Application::getGlobalHintsUpdateEvent()->fire();
						Settings *settings = Settings::GetInstance();
						settings->AddRegisteredHost(host);
						if(dialog->manual_host_id >= 0)
						{
							auto it = settings->GetManualHosts().find(dialog->manual_host_id);
							if(it != settings->GetManualHosts().end())
							{
								ManualHost h = it->second;
								h.Register(host);
								settings->AddManualHost(h);
							}
						}
						settings->WriteFile();
						dialog->app->HostsUpdated();
					});
					break;
				}
				case CHIAKI_REGIST_EVENT_TYPE_FINISHED_FAILED:
					dialog->app->Post([dialog, alive]() {
						if(alive.expired())
							return;
						dialog->setSubtitle("Failed. Check logs below.", "");
						dialog->updateActionHint(brls::Key::B, "Close");
					});
					break;
				default:
					break;
			}
		}
};


static void RegistExecuteDialogLogCb(ChiakiLogLevel level, const char *msg, void *user)
{
	chiaki_log_cb_print(level, msg, nullptr);
	auto dialog = reinterpret_cast<RegistExecuteDialog *>(user);
	RegistExecuteDialogPrivate::Log(dialog, level, msg);
}

static void RegistExecuteDialogRegistCb(ChiakiRegistEvent *event, void *user)
{
	auto dialog = reinterpret_cast<RegistExecuteDialog *>(user);
	RegistExecuteDialogPrivate::RegistEvent(dialog, event);
}
