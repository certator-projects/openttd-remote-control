/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_execution_context.h Helper classes for executing script API calls in RPC handlers */

#ifndef SCRIPT_EXECUTION_CONTEXT_H
#define SCRIPT_EXECUTION_CONTEXT_H

#include "../../script/api/script_object.hpp"
#include "../../script/script_suspend.hpp"
#include "../../core/backup_type.hpp"
#include "../../game/game.hpp"
#include "../../game/game_instance.hpp"
#include "../../company_type.h"

/**
 * RAII helper class that sets up the proper execution context for Script API calls.
 *
 * This class handles three critical setup steps required for Script API write operations:
 * 1. Sets OWNER_DEITY company mode (allows god-mode operations)
 * 2. Activates the game script instance (makes Script API calls functional)
 * 3. Automatically restores previous state on destruction (RAII pattern)
 *
 * Usage:
 * @code
 * ScriptExecutionContext ctx;
 * if (!ctx.IsValid()) {
 *     // Game instance not available
 *     reply_.set_success(false);
 *     return;
 * }
 * // Now safe to call Script API methods
 * @endcode
 */
class ScriptExecutionContext
{
public:
	/**
	 * Constructor - sets up deity mode and activates script instance.
	 */
	ScriptExecutionContext()
		: company_backup(_current_company, OWNER_DEITY),
		  active_instance(nullptr)
	{
		// Check if game instance is available
		GameInstance *instance = Game::GetInstance();
		if (instance != nullptr)
		{
			// Activate the script instance (makes Script API calls work)
			active_instance = new ScriptObject::ActiveInstance(*instance);
		}
	}

	/**
	 * Destructor - automatically restores previous company and script instance.
	 * RAII ensures cleanup even if exceptions are thrown.
	 */
	~ScriptExecutionContext()
	{
		// Delete active instance first (restores previous script context)
		delete active_instance;

		// Restore previous company (done automatically by Backup destructor, but explicit for clarity)
		company_backup.Restore();
	}

	/**
	 * Check if the context was successfully initialized.
	 * @return true if game instance is available and context is valid
	 */
	bool IsValid() const
	{
		return active_instance != nullptr;
	}

	// Prevent copying and moving
	ScriptExecutionContext(const ScriptExecutionContext &) = delete;
	ScriptExecutionContext &operator=(const ScriptExecutionContext &) = delete;
	ScriptExecutionContext(ScriptExecutionContext &&) = delete;
	ScriptExecutionContext &operator=(ScriptExecutionContext &&) = delete;

private:
	Backup<::CompanyID> company_backup; ///< Backs up and restores company mode
	ScriptObject::ActiveInstance *active_instance; ///< Activates script instance (nullptr if unavailable)
};

/**
 * Helper function that executes a Script API command with proper Script_Suspend handling.
 *
 * Script API write operations can throw Script_Suspend exceptions when they need to queue
 * commands for async execution. This function wraps the call in try-catch and handles
 * the exception properly.
 *
 * @tparam Func Callable type (lambda, function pointer, etc.)
 * @param func The callable that executes the Script API command
 * @return true if successful (including if Script_Suspend was thrown), false on failure
 *
 * Usage:
 * @code
 * bool success = ExecuteScriptCommand([&]() {
 *     RawText *text = new RawText(name);
 *     return ::ScriptCompany::SetName(text);
 * });
 * @endcode
 */
template <typename Func>
bool ExecuteScriptCommand(Func func)
{
	try
	{
		// Execute the provided function (should return bool)
		return func();
	}
	catch (Script_Suspend &e)
	{
		// Script API methods throw Script_Suspend for async operations
		// Handle the suspension and consider it successful (command was queued)
		Game::GetInstance()->HandleScriptSuspend(e);
		return true;
	}
}

#endif /* SCRIPT_EXECUTION_CONTEXT_H */
