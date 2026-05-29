/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file handlers/script_map.h RPC handlers for ScriptMap methods */

#ifndef ABI_RPC_PROXY_SCRIPT_MAP_H
#define ABI_RPC_PROXY_SCRIPT_MAP_H

#include "abi_rpc/script_map.pb.h"
#include "abi_rpc/script_generic.pb.h"

void HandleScriptMap_GetMapSizeX(const openttd::GetMapSizeXRequest &request, openttd::GetMapSizeXReply &response, openttd::GenericError *error);
void HandleScriptMap_GetMapSizeY(const openttd::GetMapSizeYRequest &request, openttd::GetMapSizeYReply &response, openttd::GenericError *error);
void HandleScriptMap_GetTileIndex(const openttd::GetTileIndexRequest &request, openttd::GetTileIndexReply &response, openttd::GenericError *error);

#endif /* ABI_RPC_PROXY_SCRIPT_MAP_H */
