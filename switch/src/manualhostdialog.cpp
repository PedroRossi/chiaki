// SPDX-FileCopyrightText: 2025 Florian Märkl <info@florianmaerkl.de>
// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <manualhostdialog.h>
#include <gui.h>

#include <algorithm>

ManualHostDialog::ManualHostDialog(MainApplication *app, ManualHost host)
	: brls::AppletFrame(true, true), app(app), host_id(host.GetID())
{
	auto list = new brls::List();
	setContentView(list);

	host_input = new brls::InputListItem("Host", host.GetHost(), "IP Address or Hostname of the console");
	list->addView(host_input);

	for(auto &h : app->GetSettings()->GetRegisteredHosts())
		registered_hosts.push_back(h.second);
	std::vector<std::string> registered_hosts_labels = { "Register on first connection" };
	std::transform(registered_hosts.begin(), registered_hosts.end(), std::back_inserter(registered_hosts_labels), [](const RegisteredHost &h) {
		return h.GetServerMAC().ToString() + " (" + h.GetServerNickname() + ")";
	});
	registered_host_select = new brls::SelectListItem("Console", registered_hosts_labels, 0);
	for(int i=0; i<registered_hosts.size(); i++)
		if(registered_hosts[i].GetServerMAC() == host.GetMAC())
		{
			registered_host_select->setSelectedValue(i + 1);
			break;
		}
	list->addView(registered_host_select);

	auto save_item = new brls::ListItem("Save");
	save_item->getClickEvent()->subscribe([this](brls::View *view) {
		Save();
	});
	list->addView(save_item);
}

ManualHostDialog::~ManualHostDialog()
{
}

void ManualHostDialog::Save()
{
	auto host_value = host_input->getValue();
	if(host_value == "")
	{
		brls::Application::notify("Please enter a host value");
		return;
	}
	unsigned int registered_host_i = registered_host_select->getSelectedValue();
	ManualHost host(
		host_id,
		host_value,
		registered_host_i > 0,
		registered_host_i > 0 ? registered_hosts[registered_host_i - 1].GetServerMAC() : HostMAC());
	auto settings = app->GetSettings();
	settings->AddManualHost(host);
	app->HostsUpdated();
	brls::Application::popView();
}

void ManualHostDialog::Present(MainApplication *app, ManualHost host)
{
	auto * const dialog = new ManualHostDialog(app, host);
	brls::PopupFrame::open(host.GetID() < 0 ? "Add console manually" : "Edit console", dialog);
}
