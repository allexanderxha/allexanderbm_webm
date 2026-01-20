#pragma once
#include "module.hpp"
#include "logger.hpp"

namespace web {

class AdminModule : public Module {
public:
    explicit AdminModule(ModuleManager& mgr) : mgr_(mgr) {}
    std::string name() const override;
    void register_routes(Router& router) override;
private:
    ModuleManager& mgr_;
    static LogLevel parse_level(const std::string& s);
};

}

