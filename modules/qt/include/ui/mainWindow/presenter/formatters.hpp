/*
 * @file modules/qt/include/ui/mainWindow/presenter/formatters.hpp
 * @brief Static formatting utilities for display values.
 */

#ifndef BLTZAR_QT_MAINWINDOW_PRESENTER_FORMATTERS_HPP
#define BLTZAR_QT_MAINWINDOW_PRESENTER_FORMATTERS_HPP

#include <string>

namespace bltzr_qt::mainWindow::presenter {

/**
 * @brief Static formatting utilities for telemetry and numeric values.
 */
class Formatters final {
public:
    /**
     * @brief Format a floating-point value with fixed precision.
     * @param value Value to format
     * @param precision Number of decimal places
     * @return Formatted string
     */
    static std::string fixedLabel(float value, int precision);

    /**
     * @brief Format bytes as human-readable string (B, KB, MB, GB).
     * @param bytes Byte count
     * @return Formatted string
     */
    static std::string bytesLabel(long bytes);

    /**
     * @brief Format age in milliseconds (e.g., "512ms" or "n/a").
     * @param ageMs Age value or max uint32_t for missing
     * @return Formatted string
     */
    static std::string ageLabel(std::uint32_t ageMs);

    /**
     * @brief Format latency in milliseconds from input.
     * @param snapshotLatencyMs Snapshot latency or max uint32_t
     * @param pipelineLatencyMs Pipeline latency or max uint32_t
     * @return Formatted string
     */
    static std::string latencyLabel(std::uint32_t snapshotLatencyMs,
                                    std::uint32_t pipelineLatencyMs);

    /**
     * @brief Format state as display string.
     * @param stateValue State enum value
     * @return Formatted string
     */
    static std::string stateLabel(int stateValue);
};

}  // namespace bltzr_qt::mainWindow::presenter

#endif  // BLTZAR_QT_MAINWINDOW_PRESENTER_FORMATTERS_HPP
