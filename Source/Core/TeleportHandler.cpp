#include <thread>
#include <chrono>
#include "TeleportHandler.hpp"
#include "../Roblox/Offsets/Offsets.h"
#include "../Roblox/Offsets/Funcs.h"
#include "../Roblox/Environment/Environment.h"
#include "../Roblox/Environment/Instances.h"
#include "Execution/Execution.h"

template<typename T>
static bool safe_read(uintptr_t address, uintptr_t offset, T* out) {
    if (!address) return false;
    __try {
        *out = *reinterpret_cast<T*>(address + offset);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void connection(lua_State* L);
void flagfix();

TeleportHandler::TeleportHandler() 
{
    isloaded = false;
    LoadedDatamodel = 0;
}

void TeleportHandler::load() 
{
    std::thread(&TeleportHandler::loop, this).detach();
}

void TeleportHandler::loop() {
    const auto idlePoll = std::chrono::milliseconds(1000);
    const auto attachPoll = std::chrono::milliseconds(250);
    const auto minRetry = std::chrono::milliseconds(250);
    const auto maxRetry = std::chrono::milliseconds(4000);
    constexpr int maxSetupFaults = 3;
    const auto faultCooldown = std::chrono::seconds(20);

    uintptr_t faultingDataModel = 0;
    int setupFaults = 0;
    auto faultCooldownUntil = std::chrono::steady_clock::now();
    auto retryDelay = minRetry;

    while (true) {
        uintptr_t fakeDataModel = 0;
        if (!safe_read(Main::Miscellaneous::FakeDatamodelPOINTER, 0, &fakeDataModel) || !fakeDataModel) {
            if (isloaded) {
                isloaded = false;
                LoadedDatamodel = 0;
            }
            std::this_thread::sleep_for(attachPoll);
            continue;
        }

        uintptr_t DataModel = 0;
        if (!safe_read(fakeDataModel, Main::Offsets::DataModel, &DataModel) || !DataModel) {
            if (isloaded) {
                isloaded = false;
                LoadedDatamodel = 0;
            }
            std::this_thread::sleep_for(attachPoll);
            continue;
        }

        if (DataModel != LoadedDatamodel) {
            isloaded = false;
            LoadedDatamodel = DataModel;
        }

        int State = 0;
        if (!safe_read(DataModel, Main::Offsets::GameLoaded, &State))
            State = 0;

        if (State != 31) {
            if (isloaded) {
                isloaded = false;
                LoadedDatamodel = 0;
            }
            std::this_thread::sleep_for(attachPoll);
            continue;
        }

        if (isloaded) {
            std::this_thread::sleep_for(idlePoll);
            continue;
        }

        uintptr_t children = 0;
        if (!safe_read(DataModel, Main::Offsets::Children, &children) || !children)
            continue;

        uintptr_t firstChild = 0;
        if (!safe_read(children, 0, &firstChild) || !firstChild)
            continue;

        uintptr_t scriptContext = 0;
        if (!safe_read(firstChild, Main::Offsets::ScriptContext, &scriptContext) || !scriptContext)
            continue;

        if (DataModel != faultingDataModel) {
            faultingDataModel = 0;
            setupFaults = 0;
        }

        if (setupFaults >= maxSetupFaults) {
            if (std::chrono::steady_clock::now() < faultCooldownUntil) {
                std::this_thread::sleep_for(idlePoll);
                continue;
            }
            setupFaults = 0;
            faultingDataModel = 0;
        }

        lua_State* skidstate = laustate::GetLuasState(scriptContext);
        if (!skidstate) {
            ++setupFaults;
            if (setupFaults >= maxSetupFaults) {
                faultCooldownUntil = std::chrono::steady_clock::now() + faultCooldown;
                faultingDataModel = DataModel;
            }
            std::this_thread::sleep_for(retryDelay);
            retryDelay = retryDelay * 2 > maxRetry ? maxRetry : retryDelay * 2;
            continue;
        }

        Execution::skidsstate = skidstate;

        flagfix();
        env::Register(skidstate);

        lua_getglobal(skidstate, "print");
        lua_pushstring(skidstate, "(your shitsploit name) has handled teleporting sir");
        lua_pcall(skidstate, 1, 0, 0);

        std::thread(connection, skidstate).detach();

        isloaded = true;
        retryDelay = minRetry;
        faultingDataModel = 0;
        setupFaults = 0;

        std::this_thread::sleep_for(idlePoll);
    }
}
