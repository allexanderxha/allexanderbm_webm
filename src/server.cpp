#include "server.hpp"
#include <cstring>
#include <string>
#include <chrono>
#include <string_view>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using socket_t = SOCKET;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using socket_t = int;
#endif

namespace web {

static void close_socket(socket_t s) {
#if defined(_WIN32)
    closesocket(s);
#else
    close(s);
#endif
}

static bool init_platform() {
#if defined(_WIN32)
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2,2), &wsa) == 0;
#else
    return true;
#endif
}

static void cleanup_platform() {
#if defined(_WIN32)
    WSACleanup();
#endif
}

Server::Server(const std::string& host, uint16_t port, const Router& router)
    : host_(host), port_(port), router_(router) {}

void Server::start() {
    if (!init_platform()) return;

    listen_fd_ =
#if defined(_WIN32)
        static_cast<long long>(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
#else
        static_cast<long long>(::socket(AF_INET, SOCK_STREAM, 0));
#endif
    if (listen_fd_ < 0) return;

    int opt = 1;
#if defined(_WIN32)
    setsockopt(static_cast<socket_t>(listen_fd_), SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(static_cast<socket_t>(listen_fd_), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    inet_pton(AF_INET, host_.c_str(), &addr.sin_addr);
    if (::bind(static_cast<socket_t>(listen_fd_), (sockaddr*)&addr, sizeof(addr)) != 0) {
        close_socket(static_cast<socket_t>(listen_fd_));
        cleanup_platform();
        return;
    }
    if (::listen(static_cast<socket_t>(listen_fd_), SOMAXCONN) != 0) {
        close_socket(static_cast<socket_t>(listen_fd_));
        cleanup_platform();
        return;
    }

    running_ = true;
    unsigned threads = (std::max)(2u, std::thread::hardware_concurrency());
    for (unsigned i = 0; i < threads; ++i) {
        workers_.emplace_back(&Server::worker_loop, this);
    }
    std::thread(&Server::accept_loop, this).detach();
    Logger::instance().log(LogLevel::Info, "Listening on " + host_ + ":" + std::to_string(port_));
}

void Server::stop() {
    running_ = false;
    q_cv_.notify_all();
    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
    if (listen_fd_ >= 0) {
        close_socket(static_cast<socket_t>(listen_fd_));
        listen_fd_ = -1;
    }
    cleanup_platform();
}

void Server::accept_loop() {
    while (running_) {
        sockaddr_in caddr{};
        #if defined(_WIN32)
        int clen = sizeof(caddr);
        #else
        socklen_t clen = sizeof(caddr);
        #endif
        socket_t c = ::accept(static_cast<socket_t>(listen_fd_), (sockaddr*)&caddr, &clen);
        if (c == socket_t(-1)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        char ipbuf[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &caddr.sin_addr, ipbuf, sizeof(ipbuf));
        std::string remote = std::string(ipbuf) + ":" + std::to_string(ntohs(caddr.sin_port));
        {
            std::lock_guard<std::mutex> lk(q_mtx_);
            q_.push(WorkItem{static_cast<long long>(c), remote});
        }
        q_cv_.notify_one();
    }
}

void Server::worker_loop() {
    while (running_) {
        WorkItem item{ -1, "" };
        {
            std::unique_lock<std::mutex> lk(q_mtx_);
            q_cv_.wait(lk, [&]{ return !running_ || !q_.empty(); });
            if (!running_) break;
            item = q_.front();
            q_.pop();
        }
        socket_t c = static_cast<socket_t>(item.s);
        std::string buf;
        buf.resize(8192);
        int total = 0;
        bool header_done = false;
        static std::atomic<unsigned long long> rid{0};
        unsigned long long req_id = ++rid;
        auto t0 = std::chrono::steady_clock::now();
        while (true) {
#if defined(_WIN32)
            int n = ::recv(c, buf.data() + total, int(buf.size() - total), 0);
#else
            int n = ::recv(c, buf.data() + total, buf.size() - total, 0);
#endif
            if (n <= 0) break;
            total += n;
            if (total >= 4) {
                auto pos = std::string_view(buf.data(), total).find("\r\n\r\n");
                if (pos != std::string::npos) {
                    header_done = true;
                    break;
                }
            }
            if (total == (int)buf.size()) buf.resize(buf.size() * 2);
        }
        std::string data(buf.data(), total);
        Response resp;
        if (!header_done) {
            resp.status = 400;
            resp.body = "Bad Request";
            resp.headers["Content-Type"] = "text/plain; charset=utf-8";
        } else {
            auto req = parse_request(data);
            resp = router_.route(req);
            resp.headers["Connection"] = "close";
            resp.headers["X-Request-ID"] = std::to_string(req_id);
            auto t1 = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
            Logger::instance().log(LogLevel::Info, req.method + " " + req.raw_target + " -> " + std::to_string(resp.status) + " " + std::to_string(resp.body.size()) + "B " + std::to_string(ms) + "ms " + item.remote);
        }
        auto out = resp.to_string();
        size_t sent = 0;
        while (sent < out.size()) {
#if defined(_WIN32)
            int n = ::send(c, out.data() + sent, int(out.size() - sent), 0);
#else
            ssize_t n = ::send(c, out.data() + sent, out.size() - sent, 0);
#endif
            if (n <= 0) break;
            sent += n;
        }
        close_socket(c);
    }
}

}
