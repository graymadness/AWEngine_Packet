#pragma once

#include <vector>
#include <string>

#include "AWEngine/Packet/asio.hpp"

namespace AWEngine::Packet::Util
{
    class DNS
    {
    public:
        DNS() = delete;

    public:
        [[nodiscard]] inline static std::vector<asio::ip::tcp::endpoint> ResolveHosts(const std::string& host, uint16_t port);
        [[nodiscard]] inline static             asio::ip::tcp::endpoint  ResolveHost(const std::string& host, uint16_t port);
    };

    std::vector<asio::ip::tcp::endpoint> DNS::ResolveHosts(const std::string& host, uint16_t port)
    {
        static asio::io_service io_service;
        static asio::ip::tcp::resolver resolver(io_service);

        std::string port_s = std::to_string(port);
        auto results = resolver.resolve(host, port_s);

        if(results.empty())
            throw std::runtime_error("Failed to resolve host '" + host + "' at port " + std::to_string(port));

        std::vector<asio::ip::tcp::endpoint> endpoints;
        endpoints.resize(results.size());
        for(const auto& r : results)
            endpoints.emplace_back(r.endpoint());
        return endpoints;
    }

    asio::ip::tcp::endpoint DNS::ResolveHost(const std::string& host, uint16_t port)
    {
        static asio::io_service io_service;
        static asio::ip::tcp::resolver resolver(io_service);

        std::string port_s = std::to_string(port);
        auto results = resolver.resolve(host, port_s);

        if(results.empty())
            throw std::runtime_error("Failed to resolve host '" + host + "' at port " + std::to_string(port));

        return results->endpoint();
    }
}
