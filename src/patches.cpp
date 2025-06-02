/**
 * =============================================================================
 * CS2Fixes
 * Copyright (C) 2023-2025 Source2ZE
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

#include "patches.h"
#include "addresses.h"
#include "common.h"
#include "icvar.h"
#include "irecipientfilter.h"
#include "mempatch.h"

#include "tier0/memdbgon.h"

extern CGameConfig* g_GameConfig;

CMemPatch g_CommonPatches[] =
	{
		CMemPatch("CheckJumpButtonWater", "FixWaterFloorJump"),
		CMemPatch("WaterLevelGravity", "WaterLevelGravity"),
		CMemPatch("BotNavIgnore", "BotNavIgnore"),
#ifndef _WIN32
		// Linux checks for the nav mesh in each bot_add command, so we patch 3 times
		CMemPatch("BotNavIgnore", "BotNavIgnore"),
		CMemPatch("BotNavIgnore", "BotNavIgnore"),
#endif
};


bool InitPatches(CGameConfig* g_GameConfig)
{
	bool success = true;

	// Skip first patch (movement unlocker), it gets patched in convar callback
	for (int i = 01; i < sizeof(g_CommonPatches) / sizeof(*g_CommonPatches); i++)
		if (!g_CommonPatches[i].PerformPatchTest(g_GameConfig))
			success = false;

	return success;
}

void UndoPatches()
{
	for (int i = 0; i < sizeof(g_CommonPatches) / sizeof(*g_CommonPatches); i++)
		g_CommonPatches[i].UndoPatch();
}