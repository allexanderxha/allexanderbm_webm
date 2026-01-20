#pragma once
#include "module.hpp"
#include "file_util.hpp"
#include "template.hpp"
#include <vector>

namespace web {

class PortfolioModule : public Module {
public:
    std::string name() const override;
    void register_routes(Router& router) override;
private:
    static std::vector<Vars> load_projects();
    static Response render_list(Router& router);
    static Response render_item(Router& router, const Request& req);
};

}

