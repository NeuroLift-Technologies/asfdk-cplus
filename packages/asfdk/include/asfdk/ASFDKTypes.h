#pragma once

#include <string>

namespace asfdk {

/**
 * @brief Result of a governance validation/wellness/checkpoint operation.
 * Kept for compatibility with the unified governance surface.
 */
struct GovernanceResult {
    bool success;
    std::string message;
    int errorCode;
};

enum class GovernanceLevel {
    Informational,
    Warning,
    Critical,
    Crisis
};

/**
 * @brief Umbrella-level error type for safe-parse entry points
 * that funnel pillar errors into a single shape for callers.
 */
struct ASFDKError {
    enum class Code {
        None,
        ParseError,
        ValidationError,
        HonorError,
        DetectionError,
        ConfigError,
        UnknownError
    };

    Code code = Code::None;
    std::string message;
};

} // namespace asfdk