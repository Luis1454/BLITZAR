#ifndef GRAVITY_RUNTIME_INCLUDE_BACKEND_BACKENDSERVER_HPP_
#define GRAVITY_RUNTIME_INCLUDE_BACKEND_BACKENDSERVER_HPP_

#include "backend/SimulationBackend.hpp"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class BackendServer {
    public:
        explicit BackendServer(SimulationBackend &backend, std::string authToken = {});
        ~BackendServer();

        bool start(std::uint16_t port, const std::string &bindAddress = "127.0.0.1");
        void stop();

        bool isRunning() const;
        bool shutdownRequested() const;

    private:
        typedef std::intptr_t SocketHandle;
        void acceptLoop();
        void handleClient(SocketHandle client);
        std::string processRequest(const std::string &request);

        SimulationBackend &_backend;
        std::atomic<bool> _running;
        std::atomic<bool> _shutdownRequested;
        std::thread _acceptThread;
        SocketHandle _listenSocket;
        std::string _bindAddress;
        std::string _authToken;
        std::uint16_t _port;
        bool _networkInitialized;
        std::mutex _socketMutex;
        std::vector<std::thread> _clientThreads;
};



#endif // GRAVITY_RUNTIME_INCLUDE_BACKEND_BACKENDSERVER_HPP_
