#pragma once
#include <string>
#include <memory>
#include <mutex>
#include <optional>
#include <CommonUtilities/mt/Thread.h>
#include "../iact/ActionClient.h"
#include <boost/process/v2/process.hpp>
#include <boost/process/v2/stdio.hpp>
#include "../../../FlashInjectorLibrary/Extension/OverlayConfig.h"

namespace p2c::kern
{
    namespace bp2 = boost::process::v2;
    namespace as = boost::asio;

    class InjectorComplex
    {
        class InjectorModule_
        {
        public:
            InjectorModule_(bool is32Bit);
            void UpdateConfig(const GfxLayer::Extension::OverlayConfig& cfg);
            void ChangeTarget(std::optional<std::string> targetModuleName);

        private:
            void SpawnReadPidTask_();
            void PushConfig_();
            as::io_context ioctx_;
            as::readable_pipe                  pipeOut_;    // child's stdout to us
            as::writable_pipe                  pipeIn_;     // us to child's stdin
            std::optional<bp2::process>        injectorProcess_;
            as::streambuf                      readBuffer_;
            as::streambuf                      writeBuffer_;
            std::jthread                       listenerThread_;
            std::mutex                         actionClientMutex_;
            std::optional<iact::ActionClient>  injectionPointClient_;
            GfxLayer::Extension::OverlayConfig config_;
        };
    public:
        void SetActive(bool active);
        bool IsActive(bool lock = true) const;
        void UpdateConfig(const GfxLayer::Extension::OverlayConfig& cfg);
        void ChangeTarget(std::optional<std::string> targetModuleName);
    private:
        mutable std::mutex mtx_;
        std::optional<std::string> targetModuleName_;
        std::unique_ptr<InjectorModule_> pInjector32_;
        std::unique_ptr<InjectorModule_> pInjector64_;
    };
}