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
            void SpawnReadErrTask_();
            void PushConfig_();
            bool                               is32Bit_;
            as::io_context                     ioctx_;
            as::writable_pipe                  pipeIn_;     // us to child's stdin
            as::readable_pipe                  pipeOut_;    // child's stdout to us
            as::readable_pipe                  pipeErr_;    // child's stderr to us
            as::streambuf                      readBuffer_;
            as::streambuf                      errBuffer_;
            std::jthread                       listenerThread_;
            std::jthread                       errListenerThread_;
            std::mutex                         actionClientMutex_;
            std::optional<iact::ActionClient>  injectionPointClient_;
            GfxLayer::Extension::OverlayConfig config_;
            bp2::process       injectorProcess_;
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