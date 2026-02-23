#ifndef GRAVITY_SIM_BACKENDSERVER_HPP
#define GRAVITY_SIM_BACKENDSERVER_HPP

#include "sim/SimulationBackend.hpp"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class BackendServer {
    public:
        explicit BackendServer(SimulationBackend &backend);
        ~BackendServer();

        bool start(std::uint16_t port, const std::string &bindAddress = "127.0.0.1");
        void stop();

        bool isRunning() const;
        bool shutdownRequested() const;

    private:
        using SocketHandle = std::intptr_t;

        void acceptLoop();
        void handleClient(SocketHandle client);
        std::string processRequest(const std::string &request);

        SimulationBackend &_backend;
        std::atomic<bool> _running;
        std::atomic<bool> _shutdownRequested;
        std::thread _acceptThread;
        SocketHandle _listenSocket;
        std::string _bindAddress;
        std::uint16_t _port;
        bool _networkInitialized;
        std::mutex _socketMutex;
        std::vector<std::thread> _clientThreads;
};

#endif // GRAVITY_SIM_BACKENDSERVER_HPP
