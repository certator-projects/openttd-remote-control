#include "pending_deferred_calls.h"
#include "call_base.h"

#include <algorithm>
#include <vector>

static std::vector<CallBase *> g_pending_deferred_calls;

void RegisterPendingDeferredCall(CallBase *call)
{
	g_pending_deferred_calls.push_back(call);
}

void UnregisterPendingDeferredCall(CallBase *call)
{
	auto it = std::find(g_pending_deferred_calls.begin(), g_pending_deferred_calls.end(), call);
	if (it != g_pending_deferred_calls.end())
	{
		g_pending_deferred_calls.erase(it);
	}
}

void PollPendingDeferredGrpcCalls()
{
	/* Copy so Proceed() can mutate the pending list. */
	std::vector<CallBase *> pending = g_pending_deferred_calls;
	for (CallBase *call : pending)
	{
		call->Proceed(true);
	}
}
