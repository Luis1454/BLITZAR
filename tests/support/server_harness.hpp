/*
 * @file tests/support/server_harness.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#ifndef GRAVITY_TESTS_SUPPORT_SERVER_HARNESS_HPP_
#define GRAVITY_TESTS_SUPPORT_SERVER_HARNESS_HPP_
#include "platform/PlatformProcess.hpp"
#include <cstdint>
#include <string>
#include <vector>

/*
 * @brief Defines the real server harness type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class RealServerHarness {
public:
    RealServerHarness() = default;
    /*
     * @brief Documents the ~real server harness operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    ~RealServerHarness();
    RealServerHarness(const RealServerHarness&) = delete;
    RealServerHarness& operator=(const RealServerHarness&) = delete;
    RealServerHarness(RealServerHarness&&) = delete;
    RealServerHarness& operator=(RealServerHarness&&) = delete;
    /*
     * @brief Documents the start operation contract.
     * @param outError Input value used by this contract.
     * @param preferredPort Input value used by this contract.
     * @param authToken Input value used by this contract.
     * @param extraArgs Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool start(std::string& outError, std::uint16_t preferredPort = 0u,
               const std::string& authToken = {}, const std::vector<std::string>& extraArgs = {});
    /*
     * @brief Documents the stop operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void stop();
    /*
     * @brief Documents the is running operation contract.
     * @param None This contract does not take explicit parameters.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool isRunning() const;
    /*
     * @brief Documents the port operation contract.
     * @param None This contract does not take explicit parameters.
     * @return std::uint16_t value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    std::uint16_t port() const;
    /*
     * @brief Documents the executable path operation contract.
     * @param None This contract does not take explicit parameters.
     * @return const std::string& value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    const std::string& executablePath() const;

private:
    /*
     * @brief Documents the resolve server executable operation contract.
     * @param None This contract does not take explicit parameters.
     * @return std::string value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static std::string resolveServerExecutable();
    /*
     * @brief Documents the is port bindable operation contract.
     * @param port Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    static bool isPortBindable(std::uint16_t port);
    /*
     * @brief Documents the wait until ready operation contract.
     * @param outError Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool waitUntilReady(std::string& outError) const;
    grav_platform::ProcessHandle _process;
    std::uint16_t _port = 0u;
    std::string _executable;
    std::string _authToken;
};
#endif // GRAVITY_TESTS_SUPPORT_SERVER_HARNESS_HPP_
