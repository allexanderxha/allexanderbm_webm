#pragma once
#include "router.hpp"
#include <memory>
#include <vector>
#include <string>

namespace web {

class Module {
public:
    virtual ~Module() = default;
    virtual std::string name() const = 0;
    virtual void register_routes(Router& router) = 0;
};

class ModuleManager {
public:
    void add(std::unique_ptr<Module> m);
    void init(Router& router);
    void load_from_config(Router& router);
    std::vector<std::string> list_names() const;
private:
    std::vector<std::unique_ptr<Module>> mods_;
};

}
