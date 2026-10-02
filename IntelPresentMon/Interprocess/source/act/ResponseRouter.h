#pragma once
#include "../../../CommonUtilities/pipe/Pipe.h"
#include "Packet.h"
#include <cstdint>

namespace pmon::ipc::act
{
	namespace as = boost::asio;

	// One in-flight request that this side of a session has sent and is waiting on.
	// Instances live on the requesting coroutine's frame for exactly as long as they
	// are registered, so the router only ever holds a pointer.
	class PendingResponse
	{
	public:
		virtual ~PendingResponse() = default;
		// Invoked on the reader coroutine when the matching response arrives. The
		// implementation knows the concrete response type and must drain the payload
		// before returning, because those bytes are sitting in the pipe read buffer
		// and the reader loop issues its next read as soon as this call completes.
		void Complete(const PacketHeader& header, util::pipe::DuplexPipe& pipe)
		{
			settled_ = true;
			OnComplete_(header, pipe);
		}
		// Invoked when the session ends before a response arrives. Every path that can
		// end a session must reach this, or the requester waits forever.
		void Fail(std::exception_ptr error)
		{
			settled_ = true;
			OnFail_(std::move(error));
		}
		// The router removes an entry from its map before settling it, and settling a
		// waiter is the last thing a dying session does. So once this is true the
		// registration must not touch the router again: it may already be destroyed.
		bool IsSettled() const
		{
			return settled_;
		}
	protected:
		virtual void OnComplete_(const PacketHeader& header, util::pipe::DuplexPipe& pipe) = 0;
		virtual void OnFail_(std::exception_ptr error) = 0;
	private:
		bool settled_ = false;
	};

	// The subset of a connector that a request coroutine needs: somewhere to write and
	// a registry that routes the eventual response back to it. Kept as an interface so
	// the transfer layer does not have to know the connector template.
	class ResponseRouter
	{
	public:
		virtual ~ResponseRouter() = default;
		virtual void RegisterPending(uint32_t commandToken, PendingResponse& pending) = 0;
		// Registered -> Expired. The token leaves the pending map and is retained in a
		// bounded expiration history. The first late response consumes that record
		// (Expired -> LateResponseConsumed). A further response is a protocol violation.
		virtual void ExpirePending(uint32_t commandToken) = 0;
		// Next routing token that is neither pending nor retained as expired.
		// Tokens are routing ids, not authority values. Called on the action io thread.
		virtual uint32_t AllocateCommandToken() = 0;
		virtual util::pipe::DuplexPipe& GetPipe() = 0;
		virtual as::io_context& GetIoContext() = 0;
	};

	// Guarantees that a token cannot be left registered by an exception on any path
	// between writing a request and consuming its response.
	class PendingRegistration
	{
	public:
		PendingRegistration(ResponseRouter& router, uint32_t commandToken, PendingResponse& pending)
			:
			router_{ router },
			pending_{ pending },
			commandToken_{ commandToken }
		{
			router_.RegisterPending(commandToken_, pending_);
		}
		PendingRegistration(const PendingRegistration&) = delete;
		PendingRegistration& operator=(const PendingRegistration&) = delete;
		PendingRegistration(PendingRegistration&&) = delete;
		PendingRegistration& operator=(PendingRegistration&&) = delete;
		~PendingRegistration()
		{
			if (!pending_.IsSettled()) {
				router_.ExpirePending(commandToken_);
			}
		}
	private:
		ResponseRouter& router_;
		PendingResponse& pending_;
		uint32_t commandToken_;
	};
}
