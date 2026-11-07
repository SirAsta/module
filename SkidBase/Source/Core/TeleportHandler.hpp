#pragma once
#include <memory>
#include <cstdint>

class TeleportHandler {
private:
    bool isloaded;
    uintptr_t LoadedDatamodel;
    void loop();

public:
    TeleportHandler();  
    void load();  
};

inline auto Teleport = std::make_unique<TeleportHandler>();
