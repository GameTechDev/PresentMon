#pragma once

namespace pmon::ipc::act
{
	// Server-originated sends declare a policy in the type.
	// There is no implicit "first session wins" selection.

	// PresentMon service: clients request, the service responds. No server send.
	struct RequestResponseServerPolicy
	{
		static constexpr bool kUnaddressedSend = false;
		static constexpr bool kAddressedSend = false;
		static constexpr bool kSinglePeer = false;
	};

	// Kernel/UI channel: at most one active peer, and unaddressed sends target it.
	struct SinglePeerServerPolicy
	{
		static constexpr bool kUnaddressedSend = true;
		static constexpr bool kAddressedSend = false;
		static constexpr bool kSinglePeer = true;
	};

	// A send names the session id returned when that session was admitted.
	struct AddressedMultiPeerServerPolicy
	{
		static constexpr bool kUnaddressedSend = false;
		static constexpr bool kAddressedSend = true;
		static constexpr bool kSinglePeer = false;
	};
}
