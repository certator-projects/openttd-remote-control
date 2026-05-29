/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file handlers/script_map.cpp RPC handlers for ScriptMap methods */

#include "script_map.h"

#include "../../../script/api/script_map.hpp"

void HandleScriptMap_GetMapSizeX(const openttd::GetMapSizeXRequest &request, openttd::GetMapSizeXReply &response, openttd::GenericError *error)
{
	(void)request;
	(void)error;
	response.set_size_x(static_cast<int32_t>(::ScriptMap::GetMapSizeX()));
}

void HandleScriptMap_GetMapSizeY(const openttd::GetMapSizeYRequest &request, openttd::GetMapSizeYReply &response, openttd::GenericError *error)
{
	(void)request;
	(void)error;
	response.set_size_y(static_cast<int32_t>(::ScriptMap::GetMapSizeY()));
}

void HandleScriptMap_GetTileIndex(const openttd::GetTileIndexRequest &request, openttd::GetTileIndexReply &response, openttd::GenericError *error)
{
	(void)error;
	::TileIndex tile_index = ::ScriptMap::GetTileIndex(request.x(), request.y());
	response.set_tile_index(tile_index.base());
}
