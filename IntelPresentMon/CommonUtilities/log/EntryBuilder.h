#pragma once
#include "Entry.h"
#include <format>
#include <memory>
#include <sstream>
#include <exception>
#if __has_include(<cereal/archives/json.hpp>)
#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>
#define PMLOG_HAS_CEREAL_JSON 1
#else
#define PMLOG_HAS_CEREAL_JSON 0
#endif
#include "TimePoint.h"
#include "PanicLogger.h"

namespace pmon::util::log
{
	class IEntrySink;

	// provide a stream-like interface for the log entry builder (useful for shimming glog etc.)
	class EntryStream : public std::ostringstream
	{
	public:
		EntryStream(class EntryBuilder& builder);
		~EntryStream();

		EntryStream(const EntryStream&) = delete;
		EntryStream & operator=(const EntryStream&) = delete;
		EntryStream(EntryStream&&) = delete;
		EntryStream & operator=(EntryStream&&) = delete;
	private:
		class EntryBuilder& builder_;
	};

	struct ex_note_type{};
	inline constexpr ex_note_type ex_note;

	// fluent wrapper interface for creating log Entries
	class EntryBuilder : private Entry
	{
	public:
		EntryBuilder(Level lvl, const char* sourceFile, const char* sourceFunctionName, int sourceLine) noexcept;
		EntryBuilder(Level lvl, std::string sourceFile, std::string, int sourceLine) noexcept;

		EntryBuilder(const EntryBuilder&) = delete;
		EntryBuilder & operator=(const EntryBuilder&) = delete;

		~EntryBuilder();
		template<typename T>
		EntryBuilder& watch(const char* symbol, const T& value) noexcept
		{
			try {
				if constexpr (std::is_pointer_v<std::remove_reference_t<T>>) {
					const void* p = static_cast<const void*>(value);
					if (note_.empty()) {
						note_ += std::format("   {} => {}", symbol, p);
					}
					else {
						note_ += std::format("\n     {} => {}", symbol, p);
					}
				}
				else {
					if (note_.empty()) {
						note_ += std::format("   {} => {}", symbol, value);
					}
					else {
						note_ += std::format("\n     {} => {}", symbol, value);
					}
				}
			}
			catch (...) { pmlog_panic_("Failed to format watch in EntryBuilder"); }
			return *this;
		}
		template<typename T>
		EntryBuilder& serialize(const char* symbol, const T& value) noexcept
		{
#if PMLOG_HAS_CEREAL_JSON
			try {
				std::ostringstream os;
				auto opts = cereal::JSONOutputArchive::Options::Default();
				cereal::JSONOutputArchive ar(os, opts);
				ar(cereal::make_nvp(symbol, value));
				return watch(symbol, os.str());
			}
			catch (const std::exception& e) {
				return watch(symbol, std::string("JSON serialization failed: ") + e.what());
			}
			catch (...) {
				return watch(symbol, std::string("JSON serialization failed"));
			}
#else
			(void)value;
			return watch(symbol, std::string("JSON serialization unavailable (cereal not built)"));
#endif
		}
		template<typename E>
		EntryBuilder& raise()
		{
			auto note = note_;
			commit_();
			throw Except<E>(std::move(note));
		}
		EntryBuilder& mark(const TimePoint& tp) noexcept;
		EntryBuilder& note(std::string note = "") noexcept;
		EntryBuilder& to(std::shared_ptr<IEntrySink>) noexcept;
		EntryBuilder& trace_skip(int depth) noexcept;
		EntryBuilder& no_trace() noexcept;
		EntryBuilder& trace() noexcept;
		EntryBuilder& hr() noexcept;
		EntryBuilder& hr(uint32_t) noexcept;
		EntryBuilder& every(int n, bool includeFirst = true) noexcept;
		EntryBuilder& first(int n) noexcept;
		EntryBuilder& after(int n) noexcept;
		EntryBuilder& hitcount() noexcept;
		EntryBuilder& diag() noexcept;
		EntryBuilder& subsys(Subsystem sys) noexcept;
		EntryStream stream() noexcept;
		template<typename C>
		EntryBuilder& code(const C& code) noexcept
		{
			errorCode_ = { code };
			return *this;
		}
		template<typename C>
		EntryBuilder& note(std::pair<std::string, std::optional<C>> noteWithCode) noexcept
		{
			auto&& [msg_, code_] = noteWithCode;
			note(std::move(msg_));
			if (code_) {
				code(*code_);
			}
			return *this;
		}
	private:
		// functions
		void commit_() noexcept;
		// data
		bool committed_ = false;
		std::shared_ptr<IEntrySink> pDest_;
		int traceSkipDepth_;
		std::optional<bool> captureTrace_;
	};

}
