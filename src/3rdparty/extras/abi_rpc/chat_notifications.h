/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file chat_notifications.h In-process chat notification queue for ABI RPC. */

#ifndef ABI_RPC_CHAT_NOTIFICATIONS_H
#define ABI_RPC_CHAT_NOTIFICATIONS_H

#include "../../../network/network_type.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace AbiRpcChatNotifications
{

static constexpr uint64_t kChatNotificationTtlMs = 60'000;
static constexpr size_t kChatNotificationStorageCapBytes = 100 * 1024; // 100 KiB

struct ChatNotification
{
	uint64_t received_unix_ms;
	uint32_t action;
	uint32_t desttype;
	uint32_t client_id;
	std::string message;
};

/**
 * Enqueue chat notification originating from the OpenTTD server.
 *
 * Notes:
 * - This is intended to be called from `NetworkAdminChat`, i.e. it only captures chat that OpenTTD would relay to the admin network.
 * - Chat originating from admins is already filtered before this call (see `NetworkAdminChat(..., from_admin)`).
 */
void Enqueue(NetworkAction action, DestType desttype, ClientID client_id, std::string_view msg, int64_t data);

/**
 * Drain all currently queued notifications into @p out and remove them from the queue.
 * Expired notifications (older than TTL) are dropped.
 */
void Drain(std::vector<ChatNotification> &out);

} // namespace AbiRpcChatNotifications

#endif /* ABI_RPC_CHAT_NOTIFICATIONS_H */
