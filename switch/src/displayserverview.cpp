// SPDX-FileCopyrightText: 2025 Florian Märkl <info@florianmaerkl.de>
// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <displayserverview.h>
#include <settings.h>
#include <registdialog.h>
#include <manualhostdialog.h>
#include <gui.h>

#include <algorithm>

#define DIALOG(dialog, r)                                                       \
	brls::Dialog *d_##dialog = new brls::Dialog(r);                             \
	brls::GenericEvent::Callback cb_##dialog = [d_##dialog](brls::View *view) { \
		d_##dialog->close();                                                    \
	};                                                                          \
	d_##dialog->addButton("Ok", cb_##dialog);                                   \
	d_##dialog->setCancelable(false);                                           \
	d_##dialog->open();                                                         \
	brls::Logger::info("Dialog: {0}", r);


DisplayServerView::DisplayServerView(MainApplication *app, const DisplayServer &server) : app(app)
{
	this->settings = Settings::GetInstance();
	this->io = IO::GetInstance();

	connect_item = new brls::ListItem("Connect");
	connect_item->getClickEvent()->subscribe([this](auto view) {
		this->app->Connect(this->server);
	});

	wakeup_item = new brls::ListItem("Wakeup");
	wakeup_item->getClickEvent()->subscribe(std::bind(&DisplayServerView::Wakeup, this, std::placeholders::_1));

	register_item = new brls::ListItem("Register");
	register_item->getClickEvent()->subscribe(std::bind(&DisplayServerView::Register, this));

	info_header_item = new brls::Label(brls::LabelStyle::REGULAR, "Information", true);
	info_state_item = new brls::ListItem("State");
	info_source_item = new brls::ListItem("Source");
	info_host_item = new brls::ListItem("Host address");
	info_mac_item = new brls::ListItem("MAC address");
	info_registered_item = new brls::ListItem("Registered");

	edit_header_item = new brls::Label(brls::LabelStyle::REGULAR, "Edit", true);
	edit_item = new brls::ListItem("Edit");
	edit_item->getClickEvent()->subscribe([this](auto view) {
		if(this->server.discovered)
			return;
		ManualHostDialog::Present(this->app, this->server.manual_host);
	});
	delete_item = new brls::ListItem("Delete");
	delete_item->getClickEvent()->subscribe([this](auto view) {
		auto dialog = new brls::Dialog("Are you sure you want to delete this host?");
		dialog->addButton("Cancel", [](auto view) {
			brls::Application::popView();
		});
		dialog->addButton("Delete", [this](auto view) {
			if(this->server.discovered)
				return;
			this->app->GetSettings()->DeleteManualHost(this->server.manual_host.GetID());
			this->app->HostsUpdated();
			brls::Application::popView();
		});
		dialog->setCancelable(true);
		dialog->open();
	});

	SetServer(server);
}

DisplayServerView::~DisplayServerView()
{
	clear(false);
	delete connect_item;
	delete wakeup_item;
	delete register_item;
	delete info_header_item;
	delete info_state_item;
	delete info_source_item;
	delete info_host_item;
	delete info_mac_item;
	delete info_registered_item;
	delete edit_header_item;
	delete edit_item;
	delete delete_item;
}

void DisplayServerView::UpdateViewVisibility()
{
	// Remember which focus views pointed to a view inside us
	std::vector<brls::View *> old_focus_views = brls::Application::getFocusStack();
	old_focus_views.push_back(brls::Application::getCurrentFocus());
	std::vector<int> old_focus_indices;
	std::transform(old_focus_views.begin(), old_focus_views.end(), std::back_inserter(old_focus_indices), [this](View *view) {
		for(size_t i=0; i<getViewsCount(); i++)
			if(getChild(i) == view)
				return (int)i;
		return -1;
	});

	// Perform view updates
	clear(false);
	if(server.registered)
	{
		addView(connect_item);
		addView(wakeup_item);
	}
	else
		addView(register_item);
	addView(info_header_item);
	if(server.discovered)
		addView(info_state_item);
	addView(info_source_item);
	addView(info_host_item);
	addView(info_mac_item);
	addView(info_registered_item);
	if(!server.discovered)
	{
		addView(edit_header_item);
		addView(edit_item);
		addView(delete_item);
	}

	// Restore focus views or fall back to a view close to the previously focused
	for(size_t i=0; i<old_focus_indices.size(); i++)
	{
		int old_index = old_focus_indices[i];
		if(old_index < 0)
			continue; // focus not inside us
		View *old_view = old_focus_views[i];
		bool focus_still_exists = false;
		for(size_t i=0; i<getViewsCount(); i++)
		{
			if(getChild(i) == old_view)
			{
				focus_still_exists = true;
				break;
			}
		}
		if(focus_still_exists)
			continue; // focused view still present, no need to change
		size_t count = getViewsCount();
		View *new_focus = count ? getChild(std::min<size_t>(count - 1, old_index)) : nullptr;
		if(i + 1 == old_focus_indices.size())
			brls::Application::giveFocus(new_focus);
		else
			brls::Application::getFocusStack()[i] = new_focus;
	}
}

void DisplayServerView::Register()
{
	if(server.registered)
		return;
	DisplayServerID id = server.GetID();
	RegistDialog::Present(app, server.GetHostAddr(), !id.discovered ? id.manual_host_id : -1);
}

void DisplayServerView::SetServer(const DisplayServer &server)
{
	this->server = server;
	info_host_item->setValue(server.GetHostAddr());
	if(server.discovered)
		info_state_item->setValue(chiaki_discovery_host_state_string(server.discovery_host.state));
	info_source_item->setValue(server.discovered ? "discovered" : "manually added");
	info_mac_item->setValue(server.discovered ? server.discovery_host.GetHostMAC().ToString()
		: server.registered ? server.registered_host.GetServerMAC().ToString()
		: "-");
	info_registered_item->setValue(server.registered ? "yes" : "no");
	UpdateViewVisibility();
}

void DisplayServerView::Wakeup(brls::View *view)
{
	if(!server.registered)
	{
		DIALOG(prypf, "Please register your PlayStation first");
		return;
	}
	ChiakiErrorCode r = app->GetDiscoveryManager()->SendWakeup(server.GetHostAddr(), server.registered_host.GetRpRegistKey(), server.IsPS5());
	if(r == 0)
		brls::Application::notify("Wakeup packet sent.");
	else
		brls::Application::notify("Failed to send wakeup packet.");
}
