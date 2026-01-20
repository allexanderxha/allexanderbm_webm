#pragma once
#include "module.hpp"
#include "http.hpp"

namespace web {

class HealthModule : public Module {
public:
    std::string name() const override;
    void register_routes(Router& router) override;
};

}

