#ifndef GRPC_SERVER_PENDING_DEFERRED_CALLS_H
#define GRPC_SERVER_PENDING_DEFERRED_CALLS_H

class CallBase;

void RegisterPendingDeferredCall(CallBase *call);
void UnregisterPendingDeferredCall(CallBase *call);
void PollPendingDeferredGrpcCalls();

#endif /* GRPC_SERVER_PENDING_DEFERRED_CALLS_H */
