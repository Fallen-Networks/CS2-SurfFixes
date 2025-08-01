/**
 * =============================================================================
 * CS2Fixes
 * Copyright (C) 2023-2024 Source2ZE
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cs2fixes.h"
#include "iserver.h"

#include "appframework/IAppSystem.h"
#include "common.h"
#include "detours.h"
#include "patches.h"
#include "icvar.h"
#include "interface.h"
#include "tier0/dbg.h"
#include "schemasystem/schemasystem.h"
#include "plat.h"
#include "entitysystem.h"
#include "engine/igameeventsystem.h"
#include "playermanager.h"
#include "gameconfig.h"
#include "entity/cgamerules.h"
#include "entity/ccsplayercontroller.h"
#include "serversideclient.h"
#include "te.pb.h"

#define VPROF_ENABLED
#include "tier0/vprof.h"

#include "tier0/memdbgon.h"

double g_flUniversalTime;
float g_flLastTickedTime;
bool g_bHasTicked;

void Message(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	char buf[1024] = {};
	V_vsnprintf(buf, sizeof(buf) - 1, msg, args);

	ConColorMsg(Color(255, 0, 255, 255), "[CS2Fixes] %s", buf);

	va_end(args);
}

void Panic(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	char buf[1024] = {};
	V_vsnprintf(buf, sizeof(buf) - 1, msg, args);

	Warning("[CS2Fixes] %s", buf);

	va_end(args);
}

class GameSessionConfiguration_t { };

SH_DECL_HOOK4_void(IServerGameClients, ClientActive, SH_NOATTRIB, 0, CPlayerSlot, bool, const char *, uint64);
SH_DECL_HOOK3_void(INetworkServerService, StartupServer, SH_NOATTRIB, 0, const GameSessionConfiguration_t&, ISource2WorldSession*, const char*);

CS2Fixes g_CS2Fixes;

IGameEventSystem *g_gameEventSystem = nullptr;
INetworkGameServer *g_pNetworkGameServer = nullptr;
CGameEntitySystem *g_pEntitySystem = nullptr;
CGlobalVars *gpGlobals = nullptr;
CPlayerManager *g_playerManager = nullptr;
IVEngineServer2 *g_pEngineServer2 = nullptr;
CGameConfig *g_GameConfig = nullptr;
CCSGameRules *g_pGameRules = nullptr;
int g_iCGamePlayerEquipUseId = -1;

CGameEntitySystem *GameEntitySystem()
{
	static int offset = g_GameConfig->GetOffset("GameEntitySystem");
	return *reinterpret_cast<CGameEntitySystem **>((uintptr_t)(g_pGameResourceServiceServer) + offset);
}

PLUGIN_EXPOSE(CS2Fixes, g_CS2Fixes);
bool CS2Fixes::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, g_pEngineServer2, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pGameResourceServiceServer, IGameResourceService, GAMERESOURCESERVICESERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pSchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, g_pSource2Server, ISource2Server, SOURCE2SERVER_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, g_pSource2ServerConfig, ISource2ServerConfig, SOURCE2SERVERCONFIG_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, g_pSource2GameEntities, ISource2GameEntities, SOURCE2GAMEENTITIES_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, g_pSource2GameClients, IServerGameClients, SOURCE2GAMECLIENTS_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, g_pNetworkServerService, INetworkServerService, NETWORKSERVERSERVICE_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, g_gameEventSystem, IGameEventSystem, GAMEEVENTSYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, g_pNetworkMessages, INetworkMessages, NETWORKMESSAGES_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);

	// Required to get the IMetamodListener events
	g_SMAPI->AddListener(this, this);

	Message( "Starting plugin.\n" );

	CBufferStringGrowable<256> gamedirpath;
	g_pEngineServer2->GetGameDir(gamedirpath);

	std::string gamedirname = CGameConfig::GetDirectoryName(gamedirpath.Get());

	const char* gamedataPath = "addons/CS2-SurfFixes/gamedata/cs2fixes.games.txt";
	Message("Loading %s for game: %s\n", gamedataPath, gamedirname.c_str());

	g_GameConfig = new CGameConfig(gamedirname, gamedataPath);
	char conf_error[255] = "";
	if (!g_GameConfig->Init(g_pFullFileSystem, conf_error, sizeof(conf_error)))
	{
		snprintf(error, maxlen, "Could not read %s: %s", g_GameConfig->GetPath().c_str(), conf_error);
		Panic("%s\n", error);
		return false;
	}

	SH_ADD_HOOK(IServerGameClients, ClientActive, g_pSource2GameClients, SH_MEMBER(this, &CS2Fixes::Hook_ClientActive), true);
	SH_ADD_HOOK(INetworkServerService, StartupServer, g_pNetworkServerService, SH_MEMBER(this, &CS2Fixes::Hook_StartupServer), true);
	META_CONPRINTF( "All hooks started!\n" );

	bool bRequiredInitLoaded = true;

	if (!addresses::Initialize(g_GameConfig))
		bRequiredInitLoaded = false;

	if (!InitPatches(g_GameConfig))
		bRequiredInitLoaded = false;

	if (!InitDetours(g_GameConfig))
		bRequiredInitLoaded = false;

	if (!bRequiredInitLoaded)
	{
		snprintf(error, maxlen, "One or more address lookups, patches or detours failed, please refer to startup logs for more information");
		return false;
	}

	if (g_GameConfig->GetOffset("CBaseEntity::Use") == -1)
	{
		snprintf(error, maxlen, "Failed to find CBaseEntity::Use\n");
		return false;
	}

	Message( "All hooks started!\n" );

	if (late)
	{
		g_pEntitySystem = GameEntitySystem();
		g_pNetworkGameServer = g_pNetworkServerService->GetIGameServer();
		gpGlobals = g_pEngineServer2->GetServerGlobals();
	}

	g_playerManager = new CPlayerManager(late);

	srand(time(0));

	return true;
}

bool CS2Fixes::Unload(char *error, size_t maxlen)
{
	SH_REMOVE_HOOK(IServerGameClients, ClientActive, g_pSource2GameClients, SH_MEMBER(this, &CS2Fixes::Hook_ClientActive), true);
	SH_REMOVE_HOOK(INetworkServerService, StartupServer, g_pNetworkServerService, SH_MEMBER(this, &CS2Fixes::Hook_StartupServer), true);

	FlushAllDetours();
	UndoPatches();

	if (g_playerManager)
		delete g_playerManager;

	if (g_GameConfig)
		delete g_GameConfig;

	if (g_iCGamePlayerEquipUseId != -1)
		SH_REMOVE_HOOK_ID(g_iCGamePlayerEquipUseId);

	return true;
}

void CS2Fixes::AllPluginsLoaded()
{
	/* This is where we'd do stuff that relies on the mod or other plugins 
	 * being initialized (for example, cvars added and events registered).
	 */

	Message( "AllPluginsLoaded\n" );
}

CUtlVector<CServerSideClient *> *GetClientList()
{
	if (!g_pNetworkGameServer)
		return nullptr;

	static int offset = g_GameConfig->GetOffset("CNetworkGameServer_ClientList");
	return (CUtlVector<CServerSideClient *> *)(&g_pNetworkGameServer[offset]);
}

CServerSideClient *GetClientBySlot(CPlayerSlot slot)
{
	CUtlVector<CServerSideClient *> *pClients = GetClientList();

	if (!pClients)
		return nullptr;

	return pClients->Element(slot.Get());
}

void CS2Fixes::Hook_ClientActive( CPlayerSlot slot, bool bLoadGame, const char *pszName, uint64 xuid )
{
	Message( "Hook_ClientActive(%d, %d, \"%s\", %lli)\n", slot, bLoadGame, pszName, xuid );
	g_playerManager->OnClientConnected(slot, xuid);
}

void CS2Fixes::Hook_StartupServer(const GameSessionConfiguration_t& config, ISource2WorldSession *pSession, const char *pszMapName)
{
	g_pNetworkGameServer = g_pNetworkServerService->GetIGameServer();
	g_pEntitySystem = GameEntitySystem();
	gpGlobals = g_pEngineServer2->GetServerGlobals();
}

bool CS2Fixes::Pause(char *error, size_t maxlen)
{
	return true;
}

bool CS2Fixes::Unpause(char *error, size_t maxlen)
{
	return true;
}

const char *CS2Fixes::GetLicense()
{
	return "GPL v3 License";
}

const char *CS2Fixes::GetVersion()
{
#ifndef CS2FIXES_VERSION
#define CS2FIXES_VERSION "1.0-dev"
#endif

	return CS2FIXES_VERSION; // defined by the build script
}

const char *CS2Fixes::GetDate()
{
	return __DATE__;
}

const char *CS2Fixes::GetLogTag()
{
	return "CS2-SurfFixes";
}

const char *CS2Fixes::GetAuthor()
{
	return "xen, Poggu, and the Source2ZE community (reduced by interesting with rampfix by zer0 and edited by Slime for the surf community)";
}

const char *CS2Fixes::GetDescription()
{
	return "Fixes For Surfing.";
}

const char *CS2Fixes::GetName()
{
	return "CS2-SurfFixes";
}

const char *CS2Fixes::GetURL()
{
	return "https://github.com/Fallen-Networks/CS2-SurfFixes";
}
