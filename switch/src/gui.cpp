// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include "gui.h"
#include <chiaki/log.h>

#include <registdialog.h>
#include <manualhostdialog.h>
#include <displayserverview.h>
#include <discoverymanager.h>
#include <streamsession.h>

#include <optional>

#define SCREEN_W 1280
#define SCREEN_H 720

// TODO
using namespace brls::i18n::literals; // for _i18n

MainApplication::MainApplication()
{
	settings = Settings::GetInstance();
	log = this->settings->GetLogger();
	io = IO::GetInstance();
	session = nullptr;
	stream_view = nullptr;
	discovery_manager = new DiscoveryManager(this, [this](auto) {
		UpdateDisplayServers();
	});
}

MainApplication::~MainApplication()
{
	delete discovery_manager;
	this->io->FreeController();
	this->io->FreeVideo();
}

bool MainApplication::Run()
{
	this->discovery_manager->SetActive(true);
	// Init the app
	brls::Logger::setLogLevel(brls::LogLevel::DEBUG);

	brls::i18n::loadTranslations();
	if(!brls::Application::init("Chiaki Remote play"))
	{
		brls::Logger::error("Unable to init Borealis application");
		return false;
	}

	// init chiaki gl after borealis
	// let borealis manage the main screen/window
	if(!io->InitVideo(0, 0, SCREEN_W, SCREEN_H))
	{
		brls::Logger::error("Failed to initiate Video");
	}

	brls::Logger::info("Load sdl/hid controller");
	if(!io->InitController())
	{
		brls::Logger::error("Faled to initiate Controller");
	}

	// Create a view
	this->tab_frame = new brls::TabFrame();
	this->tab_frame->setTitle("Chiaki: Open Source PlayStation Remote Play Client");
	this->tab_frame->setIcon(BOREALIS_ASSET("icon.png"));

	brls::List *config = new brls::List();

	BuildConfigurationMenu(config);
	config_sidebar_item = this->tab_frame->addTab("Configuration", config);
	// ----------------
	this->tab_frame->addSeparator();

	UpdateDisplayServers();

	// Add the root view to the stack
	brls::Application::pushView(this->tab_frame);

	while(brls::Application::mainLoop())
	{
		while(!queue.empty())
		{
			queue.front()();
			queue.pop_front();
		}
	}
	return true;
}

bool MainApplication::BuildConfigurationMenu(brls::List *ls)
{
	std::string psn_account_id_string = this->settings->GetPSNAccountID();
	brls::InputListItem *psn_account_id = new brls::InputListItem("PSN Account ID", psn_account_id_string,
		"Account ID in base64 format", "PS5 or PS4 v7.0 and greater", CHIAKI_PSN_ACCOUNT_ID_SIZE * 2,
		brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_SPACE |
			brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_AT |
			brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_PERCENT |
			brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_BACKSLASH);

	auto psn_account_id_cb = [this, psn_account_id](brls::View *view) {
		// retrieve, push and save setting
		this->settings->SetPSNAccountID(psn_account_id->getValue());
		// write on disk
		this->settings->WriteFile();
	};
	psn_account_id->getClickEvent()->subscribe(psn_account_id_cb);
	ls->addView(psn_account_id);

	std::string psn_online_id_string = this->settings->GetPSNOnlineID();
	brls::InputListItem *psn_online_id = new brls::InputListItem("PSN Online ID",
		psn_online_id_string, "", "", 16,
		brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_SPACE |
			brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_AT |
			brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_PERCENT |
			brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_FORWSLASH |
			brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_BACKSLASH);

	auto psn_online_id_cb = [this, psn_online_id](brls::View *view) {
		// retrieve, push and save setting
		this->settings->SetPSNOnlineID(psn_online_id->getValue());
		// write on disk
		this->settings->WriteFile();
	};
	psn_online_id->getClickEvent()->subscribe(psn_online_id_cb);
	ls->addView(psn_online_id);

	int value = 1;
	ChiakiVideoResolutionPreset resolution_preset = this->settings->GetVideoResolution();
	switch(resolution_preset)
	{
		case CHIAKI_VIDEO_RESOLUTION_PRESET_1080p:
			value = 0;
			break;
		case CHIAKI_VIDEO_RESOLUTION_PRESET_720p:
			value = 1;
			break;
		case CHIAKI_VIDEO_RESOLUTION_PRESET_540p:
			value = 2;
			break;
		case CHIAKI_VIDEO_RESOLUTION_PRESET_360p:
			value = 3;
			break;
	}

	brls::SelectListItem *resolution = new brls::SelectListItem(
		"Resolution", { "1080p (PS5 and PS4 Pro only)", "720p", "540p", "360p" }, value);

	auto resolution_cb = [this](int result) {
		ChiakiVideoResolutionPreset value = CHIAKI_VIDEO_RESOLUTION_PRESET_720p;
		switch(result)
		{
			case 0:
				value = CHIAKI_VIDEO_RESOLUTION_PRESET_1080p;
				break;
			case 1:
				value = CHIAKI_VIDEO_RESOLUTION_PRESET_720p;
				break;
			case 2:
				value = CHIAKI_VIDEO_RESOLUTION_PRESET_540p;
				break;
			case 3:
				value = CHIAKI_VIDEO_RESOLUTION_PRESET_360p;
				break;
		}
		this->settings->SetVideoResolution(value);
		this->settings->WriteFile();
	};
	resolution->getValueSelectedEvent()->subscribe(resolution_cb);
	ls->addView(resolution);

	ChiakiVideoFPSPreset fps_preset = this->settings->GetVideoFPS();
	switch(fps_preset)
	{
		case CHIAKI_VIDEO_FPS_PRESET_60:
			value = 0;
			break;
		case CHIAKI_VIDEO_FPS_PRESET_30:
			value = 1;
			break;
	}

	brls::SelectListItem *fps = new brls::SelectListItem(
		"FPS", { "60", "30" }, value);

	auto fps_cb = [this](int result) {
		ChiakiVideoFPSPreset value = CHIAKI_VIDEO_FPS_PRESET_60;
		switch(result)
		{
			case 0:
				value = CHIAKI_VIDEO_FPS_PRESET_60;
				break;
			case 1:
				value = CHIAKI_VIDEO_FPS_PRESET_30;
				break;
		}
		this->settings->SetVideoFPS(value);
		this->settings->WriteFile();
	};

	fps->getValueSelectedEvent()->subscribe(fps_cb);
	ls->addView(fps);

	auto * const add_manual_item = new brls::ListItem("Add console manually", "",
		"Add an address to connect to.");
	add_manual_item->getClickEvent()->subscribe([this](brls::View *view) {
		ManualHostDialog::Present(this, ManualHost());
	});
	ls->addView(add_manual_item);

	auto * const register_item = new brls::ListItem("Register new console", "",
		"Obtain credentials from a new console by PIN for streaming.");
	register_item->getClickEvent()->subscribe([this](brls::View *view) {
		RegistDialog::Present(this);
	});
	ls->addView(register_item);
	return true;
}

void MainApplication::HostsUpdated()
{
	UpdateDisplayServers();
}

void MainApplication::Connect(const DisplayServer &server)
{
	if(session)
		return;
	if(!server.registered)
		return;

	brls::Application::notify("Connecting to " + server.GetHostAddr() + "...");

	// ignore all user inputs (avoid double connect)
	// user inputs are restored with the CloseStream
	brls::Application::blockInputs();

	StreamSessionConnectInfo connect_info(
		io, log, settings, server.registered_host.GetTarget(), server.GetHostAddr(),
		server.registered_host.GetRpRegistKey(), server.registered_host.GetRpKey());
	try
	{
		session = new StreamSession(connect_info);
	}
	catch(Exception &e)
	{
		brls::Application::notify(e.what());
		return;
	}

	// push opengl chiaki stream
	// when the host is connected
	session->SetEventConnectedCallback([this]() {
		// https://github.com/natinusala/borealis/issues/59
		// disable 60 fps limit
		brls::Application::setMaximumFPS(0);

		// show FPS counter
		// brls::Application::setDisplayFramerate(true);

		// push raw opengl stream over borealis
		brls::Application::pushView(stream_view = new PSRemotePlay(session));
	});
	session->SetEventQuitCallback([this](ChiakiQuitEvent *event) {
		this->Post([this, event]() {
			// session QUIT call back
			brls::Application::unblockInputs();

			// restore 60 fps limit
			brls::Application::setMaximumFPS(60);

			// brls::Application::setDisplayFramerate(false);
			/*
			  DIALOG(sqrs, chiaki_quit_reason_string(quit->reason));
			*/
			brls::Application::notify(std::string("Session quit with reason: ") + chiaki_quit_reason_string(event->reason));
			Disconnect();
		});
	});
	// allow host to update controller state
	session->SetEventRumbleCallback(std::bind(&IO::SetRumble, this->io, std::placeholders::_1, std::placeholders::_2));
	session->SetReadControllerCallback(std::bind(&IO::UpdateControllerState, this->io, std::placeholders::_1, std::placeholders::_2));

	session->Start();
}

void MainApplication::Disconnect()
{
	if(stream_view)
	{
		brls::Application::popView();
		stream_view->SessionClosed();
		session->Stop();
		stream_view = nullptr;
	}

	delete session;
	session = nullptr;
}

void MainApplication::UpdateDisplayServers()
{
	display_servers.clear();

	for(const auto &host : discovery_manager->GetHosts())
	{
		DisplayServer server;
		server.discovered = true;
		server.discovery_host = host;

		server.registered = settings->GetRegisteredHostRegistered(host.GetHostMAC());
		if(server.registered)
			server.registered_host = settings->GetRegisteredHost(host.GetHostMAC());

		display_servers.push_back(server);
	}

	for(const auto &host : settings->GetManualHosts())
	{
		DisplayServer server;
		server.discovered = false;
		server.manual_host = host.second;

		server.registered = false;
		if(host.second.GetRegistered() && settings->GetRegisteredHostRegistered(host.second.GetMAC()))
		{
			server.registered = true;
			server.registered_host = settings->GetRegisteredHost(host.second.GetMAC());
		}

		display_servers.push_back(server);
	}

	UpdateDisplayServerViews();
}

static std::optional<DisplayServerID> SidebarItemDisplayServerID(brls::SidebarItem *item)
{
	if(!item)
		return std::nullopt;
	auto dsv = dynamic_cast<DisplayServerView *>(item->getAssociatedView());
	if(!dsv)
		return std::nullopt;
	return dsv->GetServer().GetID();
}

void MainApplication::UpdateDisplayServerViews()
{
	// Borealis does not handle removal of tabs like this correctly, so we
	// have to make manual adjustments.
	// We remember if any view in ...
	//   * the focus stack
	//   * the current focus
	//   * the active sidebar item
	// ... relates to a DisplayServer so we can fix these view references
	// later again.
	struct ViewMeta
	{
		DisplayServerID server;
		bool is_sidebar_item;

		ViewMeta(DisplayServerID server, bool is_sidebar_item) : server(server), is_sidebar_item(is_sidebar_item) {}
	};
	std::vector<brls::View *> old_focus_stack = brls::Application::getFocusStack();
	old_focus_stack.push_back(brls::Application::getCurrentFocus());
	std::vector<std::optional<ViewMeta>> old_focus_stack_meta;
	std::transform(old_focus_stack.begin(), old_focus_stack.end(), std::back_inserter(old_focus_stack_meta),
		[this](brls::View *view) -> std::optional<ViewMeta> {
			auto si = dynamic_cast<brls::SidebarItem *>(view);
			if(si)
			{
				auto server = SidebarItemDisplayServerID(si);
				return server ? std::optional(ViewMeta(server.value(), true)) : std::nullopt;
			}
			while(view)
			{
				auto dsv = dynamic_cast<DisplayServerView *>(view);
				if(dsv)
					return ViewMeta(dsv->GetServer().GetID(), false);
				view = view->getParent();
			}
			return std::nullopt;
		});
	std::optional<DisplayServerID> active_sidebar_item_server = SidebarItemDisplayServerID(tab_frame->getSidebar()->getCurrentActive());
	if(active_sidebar_item_server.has_value())
		tab_frame->getSidebar()->setActive(nullptr);

	// Perform view updates
	std::map<DisplayServerID, size_t> old_server_indices;
	size_t old_servers_count = server_views_ordered.size();
	for(size_t i=0; i<server_views_ordered.size(); i++)
	{
		DisplayServerView *v = server_views_ordered[i];
		old_server_indices[v->GetServer().GetID()] = i;
		this->tab_frame->removeTab(v, false);
	}
	std::map<DisplayServerID, DisplayServerView *> outdated_views = server_views;
	server_views.clear();
	for(const auto &server : display_servers)
	{
		auto id = server.GetID();
		auto view = outdated_views[id];
		outdated_views.erase(id);
		if(!view)
			view = new DisplayServerView(this, server);
		else
			view->SetServer(server);
		server_views[id] = view;
	}
	for(auto &e : outdated_views)
		delete e.second;
	server_views_ordered.clear();
	std::transform(server_views.begin(), server_views.end(), std::back_inserter(server_views_ordered), [](auto e) {
		return e.second;
	});
	std::sort(server_views_ordered.begin(), server_views_ordered.end(), [](const DisplayServerView *a, const DisplayServerView *b) {
		auto a_name = a->GetServer().GetDisplayName();
		auto b_name = b->GetServer().GetDisplayName();
		if(a_name < b_name)
			return true;
		if(b_name > a_name)
			return false;
		return a->GetServer().GetID() < b->GetServer().GetID();
	});
	std::map<DisplayServerID, brls::SidebarItem *> sidebar_items;
	std::vector<brls::SidebarItem *> sidebar_items_ordered;
	for(auto view : server_views_ordered)
	{
		auto si = tab_frame->addTab(view->GetServer().GetDisplayName(), view);
		sidebar_items[view->GetServer().GetID()] = si;
		sidebar_items_ordered.push_back(si);
	}

	// Replace any references to now deleted views
	auto NewFocusForOldServer = [&](DisplayServerID id) -> brls::SidebarItem * {
		if(sidebar_items_ordered.empty())
			return config_sidebar_item;
		auto it = old_server_indices.find(id);
		if(it == old_server_indices.end())
			return 0;
		size_t i = it->second >= sidebar_items_ordered.size() ? sidebar_items_ordered.size() - 1 : it->second;
		return sidebar_items_ordered[i];
	};
	for(size_t i=0; i<old_focus_stack_meta.size(); i++)
	{
		const auto &meta = old_focus_stack_meta[i];
		if(!meta)
			continue;
		auto ReplaceFocus = [&](brls::View *new_focus) {
			if(i + 1 == old_focus_stack_meta.size())
				brls::Application::giveFocus(new_focus);
			else
				brls::Application::getFocusStack()[i] = new_focus;
		};

		auto it = server_views.find(meta->server);
		if(it != server_views.end())
		{
			// server still exists
			if(meta->is_sidebar_item)
				ReplaceFocus(sidebar_items[meta->server]); // sidebar items must always be replaced
			continue; // DisplayServerViews and their children are not deleted, they handle updates themselves
		}
		// server does not exist anymore, fall back to index
		ReplaceFocus(NewFocusForOldServer(meta->server));
	}
	if(active_sidebar_item_server)
	{
		auto it = sidebar_items.find(*active_sidebar_item_server);
		auto si = it != sidebar_items.end() ? it->second : NewFocusForOldServer(*active_sidebar_item_server);
		tab_frame->getSidebar()->setActive(si);
		if(si)
			tab_frame->switchToView(si->getAssociatedView());
	}
}

void MainApplication::Post(std::function<void()> f)
{
	queue.push_back(f);
}

PSRemotePlay::PSRemotePlay(StreamSession *session)
	: session(session)
{
	io = IO::GetInstance();
}

void PSRemotePlay::SessionClosed()
{
	session = nullptr;
}

void PSRemotePlay::draw(NVGcontext *vg, int x, int y, unsigned width, unsigned height, brls::Style *style, brls::FrameContext *ctx)
{
	io->MainLoop();
	if(session)
		session->SendFeedbackState();
}

PSRemotePlay::~PSRemotePlay()
{
}
