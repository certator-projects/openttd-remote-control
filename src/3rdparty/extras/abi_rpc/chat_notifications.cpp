/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file chat_notifications.cpp In-process chat notification queue for ABI RPC. */

#include "chat_notifications.h"

#include <chrono>
#include <deque>
#include <mutex>

namespace AbiRpcChatNotifications
{

namespace
{

struct StoredNotification
{
	ChatNotification msg;
	std::chrono::steady_clock::time_point received_steady;
	size_t accounted_bytes;
};

std::mutex g_mutex;
std::deque<StoredNotification> g_queue;
size_t g_accounted_bytes = 0;

uint64_t NowUnixMs()
{
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

std::chrono::steady_clock::time_point NowSteady()
{
	return std::chrono::steady_clock::now();
}

void PurgeExpiredLocked(const std::chrono::steady_clock::time_point now)
{
	const auto ttl = std::chrono::milliseconds(kChatNotificationTtlMs);
	while (!g_queue.empty())
	{
		if (now - g_queue.front().received_steady <= ttl) break;
		g_accounted_bytes -= g_queue.front().accounted_bytes;
		g_queue.pop_front();
	}
}

} // namespace

void Enqueue(NetworkAction action, DestType desttype, ClientID client_id, std::string_view msg, int64_t data)
{
	auto now_steady = NowSteady();

	ChatNotification cn;
	cn.received_unix_ms = NowUnixMs();
	cn.action = static_cast<uint32_t>(action);
	cn.desttype = static_cast<uint32_t>(desttype);
	cn.client_id = static_cast<uint32_t>(client_id);
	cn.message.assign(msg.begin(), msg.end());
	(void)data;

	/*
	 * Storage cap accounting:
	 * - Count payload bytes (chat message text) plus a small fixed overhead for the record.
	 * - Cap is enforced by dropping the oldest records first.
	 */
	constexpr size_t kPerRecordOverheadBytes = 64;
	size_t accounted = cn.message.size() + kPerRecordOverheadBytes;

	std::lock_guard<std::mutex> lock(g_mutex);

	PurgeExpiredLocked(now_steady);

	g_queue.push_back(StoredNotification{std::move(cn), now_steady, accounted});
	g_accounted_bytes += accounted;

	while (g_accounted_bytes > kChatNotificationStorageCapBytes && !g_queue.empty())
	{
		g_accounted_bytes -= g_queue.front().accounted_bytes;
		g_queue.pop_front();
	}
}

void Drain(std::vector<ChatNotification> &out)
{
	auto now_steady = NowSteady();

	std::lock_guard<std::mutex> lock(g_mutex);
	PurgeExpiredLocked(now_steady);

	out.clear();
	out.reserve(g_queue.size());

	while (!g_queue.empty())
	{
		out.emplace_back(std::move(g_queue.front().msg));
		g_accounted_bytes -= g_queue.front().accounted_bytes;
		g_queue.pop_front();
	}
}

} // namespace AbiRpcChatNotifications
