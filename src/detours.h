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

#pragma once
#include "cdetour.h"

class CTriggerPush;
class CGameConfig;
class CCSPlayer_MovementServices;
class CMoveData;
class CCSPlayer_MovementServices;

bool InitDetours(CGameConfig *gameConfig);
void FlushAllDetours();

void FASTCALL Detour_ProcessMovement(CCSPlayer_MovementServices *pThis, void *pMove);
void FASTCALL Detour_TryPlayerMove(CCSPlayer_MovementServices *ms, CMoveData *mv, Vector *pFirstDest, trace_t *pFirstTrace);
void FASTCALL Detour_CategorizePosition(CCSPlayer_MovementServices *ms, CMoveData *mv, bool bStayOnGround);
void FASTCALL Detour_TriggerPush_Touch(CTriggerPush* pPush, Z_CBaseEntity* pOther);
void* FASTCALL Detour_CNavMesh_GetNearestNavArea(int64_t unk1, float* unk2, unsigned int* unk3, unsigned int unk4, int64_t unk5, int64_t unk6, float unk7, int64_t unk8);