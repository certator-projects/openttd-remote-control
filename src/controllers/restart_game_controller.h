/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0/>.
 */

/** @file controllers/restart_game_controller.h Shared restart game logic (console + RPC). */

#ifndef RESTART_GAME_CONTROLLER_H
#define RESTART_GAME_CONTROLLER_H

#include <string>
#include <string_view>

enum class RestartGameControllerStatus {
	Success,
	ErrorInvalidArgument,
};

RestartGameControllerStatus RestartGameController(std::string_view mode, std::string &out_error);

#endif /* RESTART_GAME_CONTROLLER_H */
