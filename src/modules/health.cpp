#include "health.hpp"

namespace web {

std::string HealthModule::name() const {
    return "health";
}

void HealthModule::register_routes(Router& router) {
    router.add("GET", "/health", [](const Request& req){
        Response r;
        r.status = 200;
        r.headers["Content-Type"] = "text/plain; charset=utf-8";
        r.body = "OK";
        return r;
    });
    router.add("GET", "/metrics", [](const Request& req){
        Response r;
        r.status = 200;
        r.headers["Content-Type"] = "text/plain; charset=utf-8";
        r.body = "uptime_seconds=0\nrequests_total=0";
        return r;
    });
}

}

