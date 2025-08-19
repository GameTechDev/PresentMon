#include "../CommonUtilities/win/MessageBox.h"
#include "../CommonUtilities/win/Utilities.h"
#include <memory>
#include <string>
#include <format>

#include "Context.h"
#include "Hooks/Hooks.h"
#include "Custom/Extensions.h"
#include "../FlashInjector/Logging.h"
#include "act/InjectionPointExecutionContext.h"
#include "act/Common.h"

using namespace pmon::util;
using namespace std::literals;

std::unique_ptr<inj::act::ActionServer> pServer;

// null logger factory to satisfy linking requirements for CommonUtilities
namespace pmon::util::log
{
    std::shared_ptr<class IChannel> GetDefaultChannel() noexcept
    {
        return {};
    }
}

namespace GfxLayer
{
    void WaitForUserInput()
    {
        auto message = "GfxLayer.dll has been injected to the following application: \n"s + win::GetExecutableModulePath().string() + "\n\n"s;
        message += "Press OK to continue running the application."s;
        win::MsgBox{ std::move(message) }.WithTitle("Pausing Target Application").AsModal();
    }

    void Initialize()
    {
        LOGI << "GfxLayer attached!";

        // Hook APIs
        Hooks::DXGI::Hook_DXGI();
        Hooks::D3D10::Hook_D3D10();
        Hooks::D3D11::Hook_D3D11();

        // start action server
        pServer = std::make_unique<inj::act::ActionServer>(
            inj::act::InjectionPointExecutionContext{},
            inj::act::MakePipeName(GetCurrentProcessId()), 2, ""
        );
    }
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD reason, LPVOID pReserved)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            GfxLayer::Initialize();
            break;
    }

    return TRUE;
}