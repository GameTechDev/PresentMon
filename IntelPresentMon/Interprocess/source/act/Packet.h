#pragma once
#include <string>
#include <cstdint>

namespace pmon::ipc::act
{
	enum class TransportStatus
	{
		Success,
		ExecutionFailure,
		TransportFailure,
	};

	enum class PacketType
	{
		ActionRequest,
		ActionResponse,
		ActionEvent,
	};

	// Version 2 carries the single-pipe demultiplexed protocol: requests, responses
	// and events share one full-duplex handle and are told apart by packetType plus
	// commandToken. There is no compatibility with version 1 peers.
	inline constexpr uint16_t kHeaderVersion = 2;

	// The identifier is remote input used as a map key, so its length is bounded.
	inline constexpr size_t kMaxIdentifierLength = 64;

	// Smallest body that can hold a serialized PacketHeader with an empty identifier
	// and an empty payload. MinimalPacketBodySizeMatchesProtocolFloor asserts this
	// against the real serialized size rather than trusting the constant.
	inline constexpr uint32_t kMinPacketBodyBytes = 28;

	// Generous ceiling relative to the largest real packet on either channel. Bodies
	// are rejected against this before the receive buffer is grown to hold them.
	inline constexpr uint32_t kMaxPacketBodyBytes = 1024 * 1024;

	struct PacketHeader
	{
		std::string identifier{};
		uint32_t commandToken{};
		TransportStatus transportStatus{};
		int executionStatus{};
		PacketType packetType{};
		uint16_t headerVersion{};
		uint16_t actionVersion{};

		template<class A> void serialize(A& ar) {
			ar(identifier, commandToken, transportStatus, executionStatus,
				packetType, headerVersion, actionVersion);
		}
	};

	struct EmptyPayload {};

	inline PacketHeader MakeResponseHeader(const PacketHeader& reqHeader, TransportStatus txs, int exs)
	{
		auto resHeader = reqHeader;
		// the reader loop on the far side demultiplexes on this field, so tagging the
		// response (rather than echoing the request tag) is what makes routing work
		resHeader.packetType = PacketType::ActionResponse;
		resHeader.transportStatus = txs;
		resHeader.executionStatus = exs;
		return resHeader;
	}

	// Structural screening of a freshly deserialized header. A header that fails
	// this cannot be dispatched safely, so the session that sent it is terminated.
	inline bool IsPlausibleHeader(const PacketHeader& header)
	{
		return header.headerVersion == kHeaderVersion &&
			(int)header.packetType >= (int)PacketType::ActionRequest &&
			(int)header.packetType <= (int)PacketType::ActionEvent &&
			!header.identifier.empty() &&
			header.identifier.size() <= kMaxIdentifierLength;
	}
}
