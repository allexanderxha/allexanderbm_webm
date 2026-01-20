#pragma once
#include "http.hpp"
#include "router.hpp"
#include "logger.hpp"
#include <atomic>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <string>

namespace web {

class Server {
public:
    Server(const std::string& host, uint16_t port, const Router& router);
    void start();
    void stop();
private:
    std::string host_;
    uint16_t port_;
    const Router& router_;
    std::atomic<bool> running_{false};
    std::vector<std::thread> workers_;
    std::mutex q_mtx_;
    std::condition_variable q_cv_;
    struct WorkItem { long long s; std::string remote; };
    std::queue<WorkItem> q_;
    long long listen_fd_{-1};
    void accept_loop();
    void worker_loop();
};

}
