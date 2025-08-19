#pragma once
#include "../../../CommonUtilities/win/WinAPI.h"
#include "../../../CommonUtilities/pipe/Pipe.h"
#include "../../../CommonUtilities/log/Log.h"
#include <cereal/archives/binary.hpp>
#include "Packet.h"
#include "ActionExecutionError.h"

namespace pmon::ipc::act
{
	using namespace util;

	template<class ExecCtx>
	class AsyncAction
	{
	public:
		// types
		using ExecutionContext = ExecCtx;
		using SessionContext = ExecutionContext::SessionContextType;
		// functions
		virtual ~AsyncAction() = default;
		virtual const char* GetIdentifier() const = 0;
		virtual pipe::as::awaitable<void> Execute(ExecutionContext& ctx, SessionContext& stx,
			const PacketHeader& header, pipe::DuplexPipe& pipe) const = 0;
	};

	template<class T, class ExecutionContext>
	class AsyncActionBase_ : public AsyncAction<ExecutionContext>
	{
	public:
		pipe::as::awaitable<void> Execute(ExecutionContext& ctx, AsyncAction<ExecutionContext>::SessionContext& stx,
			const PacketHeader& header, pipe::DuplexPipe& pipe) const final
		{
			PacketHeader resHeader;
			typename T::Response output;
			try {
				output = T::Execute_(ctx, stx, pipe.ConsumePacketPayload<typename T::Params>());
				resHeader = MakeResponseHeader(header, TransportStatus::Success, PM_STATUS_SUCCESS);
			}
			catch (const ActionExecutionError& e) {
				pmlog_error(std::format("Error in action [{}] execution", GetIdentifier())).code(e.GetCode());
				resHeader = MakeResponseHeader(header, TransportStatus::ExecutionFailure, e.GetCode());
			}
			catch (...) {
				pmlog_error(util::ReportException());
				// if the output buffer is dirty, we're not sure what state we're in so just clear it
				if (pipe.GetWriteBufferPending()) {
					pipe.ClearWriteBuffer();
				}
				resHeader = MakeResponseHeader(header, TransportStatus::TransportFailure, PM_STATUS_SUCCESS);
			}
			if (resHeader.transportStatus == TransportStatus::Success) {
				// if no errors occured transmit standard packet with header and action response payload
				co_await pipe.WritePacket(resHeader, output, ctx.responseWriteTimeoutMs);
			}
			else {
				// if there was an error, transmit header (configured with error status) and empty payload
				co_await pipe.WritePacket(resHeader, EmptyPayload{}, ctx.responseWriteTimeoutMs);
			}
		}
		const char* GetIdentifier() const final
		{
			return T::Identifier;
		}
		// default version for all actions
		static constexpr uint16_t Version = 1;
	};

	template<class T, class ExecutionContext>
	class AsyncEventActionBase_ : public AsyncAction<ExecutionContext>
	{
	public:
		pipe::as::awaitable<void> Execute(ExecutionContext& ctx, AsyncAction<ExecutionContext>::SessionContext& stx,
			const PacketHeader& header, pipe::DuplexPipe& pipe) const final
		{
			PacketHeader resHeader;
			try {
				T::Execute_(ctx, stx, pipe.ConsumePacketPayload<typename T::Params>());
			}
			catch (const ActionExecutionError& e) {
				pmlog_error(std::format("Error in action [{}] execution: {}", GetIdentifier(), e.what())).code(e.GetCode());
			}
			catch (...) {
				pmlog_error(util::ReportException());
			}
			co_return;
		}
		const char* GetIdentifier() const final
		{
			return T::Identifier;
		}
		// default version for all actions
		static constexpr uint16_t Version = 1;
	};

	template<class P>
	struct ActionParamsTraits;

	template<class A>
	concept Request = std::is_base_of_v<AsyncActionBase_<A, typename A::ExecutionContext>, A> && requires {
		typename A::Params;
		typename A::Response;
	};

	template<class A>
	concept Event = std::is_base_of_v<AsyncEventActionBase_<A, typename A::ExecutionContext>, A> && requires {
		typename A::Params;
	};

	template<class Params>
	using ActionFromParams = typename ActionParamsTraits<std::decay_t<Params>>::Action;

	template<class Params>
	using ResponseFromParams = typename ActionFromParams<Params>::Response;

	template<class Params>
	using AwaitableFromParams = pipe::as::awaitable<ResponseFromParams<Params>>;
}