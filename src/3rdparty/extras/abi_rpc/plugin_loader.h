/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file plugin_loader.h Plugin dynamic library loader. */

#ifndef PLUGIN_LOADER_H
#define PLUGIN_LOADER_H

/**
 * Initialize the plugin system.
 * Checks OTTD_USE_RPC_PLUGIN environment variable and loads the plugin if specified.
 * @return True if plugin was loaded successfully or no plugin specified, false on error.
 */
bool PluginLoader_Initialize();

/**
 * Start the loaded plugin's RPC server.
 * Must be called after PluginLoader_Initialize() and before HandleRPCCalls().
 * @return True if plugin was started successfully or no plugin loaded, false on error.
 */
bool PluginLoader_Start();

/**
 * Handle pending RPC calls from the plugin.
 * Called periodically from the main game loop.
 * Safe to call even if no plugin is loaded.
 */
void PluginLoader_HandleRPCCalls();

/**
 * Shutdown and unload the plugin.
 * Called during application cleanup.
 */
void PluginLoader_Shutdown();

/**
 * Check if a plugin is currently loaded.
 * @return True if a plugin is loaded, false otherwise.
 */
bool PluginLoader_IsLoaded();

#endif /* PLUGIN_LOADER_H */
