#pragma once

namespace pmon::ipc::act
{
	// Server-originated sends declare a policy in the type.
	// There is no implicit "first session wins" selection.
	// kPaired selects how sessions arrive: false, clients connect to named listeners;
	// true, there are no listeners and each session is a pipe pair the server creates
	// itself (OpenPairedSession) and hands one end of to a process it launches.

	// PresentMon service: clients request, the service responds. No server send.
	struct RequestResponseServerPolicy
	{
		static constexpr bool kUnaddressedSend = false;
		static constexpr bool kAddressedSend = false;
		static constexpr bool kSinglePeer = false;
		static constexpr bool kPaired = false;
	};

	// At most one active peer, and unaddressed sends target it.
	struct SinglePeerServerPolicy
	{
		static constexpr bool kUnaddressedSend = true;
		static constexpr bool kAddressedSend = false;
		static constexpr bool kSinglePeer = true;
		static constexpr bool kPaired = false;
	};

	// Kernel/UI channel: single peer, and that peer can only be the process the server
	// handed a pipe pair to, since no listener exists.
	struct PairedSinglePeerServerPolicy
	{
		static constexpr bool kUnaddressedSend = true;
		static constexpr bool kAddressedSend = false;
		static constexpr bool kSinglePeer = true;
		static constexpr bool kPaired = true;
	};

	// A send names the session id returned when that session was admitted.
	struct AddressedMultiPeerServerPolicy
	{
		static constexpr bool kUnaddressedSend = false;
		static constexpr bool kAddressedSend = true;
		static constexpr bool kSinglePeer = false;
		static constexpr bool kPaired = false;
	};
}
