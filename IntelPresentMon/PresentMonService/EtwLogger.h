#pragma once
#include "EtwLogSession.h"
#include <memory>
#include <mutex>
#include <unordered_map>
#include <CommonUtilities/file/SecureSubdirectory.h>

namespace pmon::svc
{
	class EtwLogger
	{
	public:
		EtwLogger(bool isElevated) noexcept;
		uint32_t StartLogSession(std::shared_ptr<EtwLogProviderListener> = {});
		util::file::TempFile FinishLogSession(uint32_t id);
		void CancelLogSession(uint32_t id);
		bool HasActiveSession(uint32_t id) const;
	private:
		// functions
		static std::shared_ptr<EtwLogProviderListener> CaptureProviderDescriptions_();
		std::shared_ptr<EtwLogProviderListener> GetDefaultProviderDescriptions_();
		static std::string MakeSessionBaseName_();
		static std::string MakeSessionName_(uint32_t id);
		static void EnsureSessionNameAvailability_(const std::string& name);
		static uint32_t GetNextSessionId_();
		// data
		static uint32_t nextSessionId_;
		util::file::SecureSubdirectory workDirectory_;
		std::shared_ptr<EtwLogProviderListener> defaultProviderDescriptionCache_;
		std::unordered_map<uint32_t, EtwLogSession> sessions_;
		mutable std::mutex mtx_;
	};
}