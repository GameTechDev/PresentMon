#pragma once
#include "../../../CommonUtilities/win/WinAPI.h"
#include "../../../CommonUtilities/pipe/Pipe.h"
#include "../../../CommonUtilities/pipe/ManualAsyncEvent.h"
#include "Packet.h"
#include "ActionExecutionError.h"
#include "AsyncAction.h"
#include "ResponseRouter.h"
#include <chrono>
#include <optional>

namespace pmon::ipc::act
{
	namespace as = boost::asio;
	using namespace util::pipe;

	PM_DEFINE_EX(ResponseTimeout);

	// How long a requester waits for its response before giving up. Sized well above the
	// slowest real action (service-side ETW session setup) so that only a peer which has
	// genuinely stopped answering trips it, but bounded so that nobody waits forever.
	inline constexpr uint32_t kDefaultResponseTimeoutMs = 10000;

	// Turns a non-Success response header into the exception the requester should see.
	inline std::exception_ptr MakeResponseError(const PacketHeader& resHeader)
	{
		if (resHeader.executionStatus) {
			const auto code = (PM_STATUS)resHeader.executionStatus;
			pmlog_error("Execution error response to SyncRequest").code(code).diag();
			return std::make_exception_ptr(util::Except<ActionExecutionError>(code));
		}
		pmlog_error("Execution error response to SyncRequest").diag();
		return std::make_exception_ptr(util::Except<util::Exception>("Execution error response to SyncRequest"));
	}

	template<Request C>
	auto SyncRequest(const typename C::Params& params, ResponseRouter& router,
		std::optional<uint32_t> writeTimeoutMs = {},
		std::optional<uint32_t> responseTimeoutMs = {}) -> as::awaitable<typename C::Response>
	{
		// Allocated on the action io thread, skipping pending and retained expired values.
		const auto commandToken = router.AllocateCommandToken();
		// the reader loop owns the pipe, so it fills this in on our behalf and wakes us;
		// a response therefore no longer has to be the next packet on the wire
		struct Completion : PendingResponse
		{
			Completion(as::io_context& ioctx) : signal{ ioctx } {}
			void OnComplete_(const PacketHeader& header, DuplexPipe& pipe) override
			{
				try {
					if (header.transportStatus == TransportStatus::Success) {
						response = pipe.ConsumePacketPayload<typename C::Response>();
					}
					else {
						// the responder wrote an empty payload; consume it to leave the stream clean
						pipe.ConsumePacketPayload<EmptyPayload>();
						error = MakeResponseError(header);
					}
				}
				catch (...) {
					error = std::current_exception();
				}
				signal.Signal();
			}
			void OnFail_(std::exception_ptr e) override
			{
				error = std::move(e);
				signal.Signal();
			}
			typename C::Response response{};
			std::exception_ptr error;
			ManualAsyncEvent signal;
		} completion{ router.GetIoContext() };

		const PacketHeader reqHeader{
			.identifier = C::Identifier,
			.commandToken = commandToken,
			.packetType = PacketType::ActionRequest,
			.headerVersion = kHeaderVersion,
			.actionVersion = C::Version,
		};
		// register before writing, otherwise a fast responder can race ahead of us
		const PendingRegistration registration{ router, commandToken, completion };
		co_await router.GetPipe().WritePacket(reqHeader, params, writeTimeoutMs);
		// on the way out the registration expires this token, so a response that arrives
		// after we have given up is dropped by the reader loop rather than killing the session
		const std::chrono::milliseconds responseTimeout{ responseTimeoutMs.value_or(kDefaultResponseTimeoutMs) };
		if (!co_await completion.signal.AsyncWait(responseTimeout)) {
			pmlog_error("Timeout waiting for action response")
				.pmwatch(C::Identifier).pmwatch(commandToken);
			throw util::Except<ResponseTimeout>("Timeout waiting for action response");
		}
		if (completion.error) {
			std::rethrow_exception(completion.error);
		}
		co_return std::move(completion.response);
	}

	template<Event C>
	auto AsyncEmit(const typename C::Params& params, ResponseRouter& router,
		std::optional<uint32_t> writeTimeoutMs = {}) -> as::awaitable<void>
	{
		const auto commandToken = router.AllocateCommandToken();
		const PacketHeader reqHeader{
			.identifier = C::Identifier,
			.commandToken = commandToken,
			.packetType = PacketType::ActionEvent,
			.headerVersion = kHeaderVersion,
			.actionVersion = C::Version,
		};
		// events are never answered, so no token is registered
		co_await router.GetPipe().WritePacket(reqHeader, params, writeTimeoutMs);
	}
}
