#ifndef BLITZAR_INTEGRATION_KDK_CHECKPOINT_HPP
#define BLITZAR_INTEGRATION_KDK_CHECKPOINT_HPP

#include "core/Types.hpp"
#include "particles/ParticleArena.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>

namespace blitzar_integration {

class KdkCheckpoint final {
public:
    explicit KdkCheckpoint(std::size_t count);
    explicit KdkCheckpoint(blitzar_particles::ParticleArena& arena);
    ~KdkCheckpoint() = default;

    KdkCheckpoint(const KdkCheckpoint&) = delete;
    KdkCheckpoint& operator=(const KdkCheckpoint&) = delete;

    KdkCheckpoint(KdkCheckpoint&& other) noexcept;
    KdkCheckpoint& operator=(KdkCheckpoint&& other) noexcept;

    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] blitzar_status SetCount(std::size_t count) noexcept;
    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] blitzar_status Capture(blitzar_core::MutableParticleView state) noexcept;
    [[nodiscard]] blitzar_status Restore(blitzar_core::MutableParticleView state) noexcept;

private:
    [[nodiscard]] bool HasArena() const noexcept;
    [[nodiscard]] blitzar_particles::ParticleArena& Arena() const noexcept;

    std::unique_ptr<blitzar_particles::ParticleArena> owned_arena_;
    std::optional<std::reference_wrapper<blitzar_particles::ParticleArena>> borrowed_arena_;
    std::size_t count_;
};

} // namespace blitzar_integration

#endif
