#pragma once

#include <string>
#include <memory>
#include <optional>
#include <vector>

// Include actual pillar headers (TOI public API is the free functions in
// TermsOfInteraction.h — TOIManager is an internal implementation detail).
#include "toi/TOITypes.h"
#include "toi/TermsOfInteraction.h"
#include "otoi/OTOIManager.h"
#include "rrt/RRTAdvocate.h"
#include "rrt/RRTTypes.h"
#include "sleepwalker/SleepwalkerProtocol.h"

#include <nlohmann/json.hpp>
#include "asfdk/ASFDKTypes.h"

namespace asfdk {

// NOTE: Pillar types are fully defined by the pillar headers included above.
// Value members (std::optional / by-value) require complete types.

struct Envelope {
    bool trusted;
    std::string channel;
    bool flagged = false;
    std::string flagReason;
    std::string consentLevel;
    nlohmann::json payload;
};

struct AssessmentResult {
    bool requiresRrtaHandoff;
    std::optional<rrt::CrisisAssessment> crisis;
    sleepwalker::EmotionalState emotionalState;
};

struct FoundationStatus {
    bool toi_active;
    bool otoi_active;
    bool rrt_active;
    bool swp_active;
    std::string otoi_mode;
    std::string overall;

    nlohmann::json toJson() const;
};

/**
 * @brief The ASFDK Umbrella class.
 * Composes the four pillars of the Solidarity Framework into a 
 * single unified interface for the application.
 */
class ASFDK {
public:
    ASFDK();
    ~ASFDK();

    // Prevent copying to ensure singleton-like behavior within a context
    ASFDK(const ASFDK&) = delete;
    ASFDK& operator=(const ASFDK&) = delete;

    // ===================== TOI surface (pass-through to toi::TOIManager) =====================
    
    toi::TOIDocument parseTOI(const nlohmann::json& json);
    std::optional<toi::TOIDocument> safeParseTOI(const nlohmann::json& json, std::string* error = nullptr);
    bool validateTOI(const toi::TOIDocument& doc);
    toi::TOIDocument resolveTOI(std::vector<toi::TOIDocument> docs);

    // ===================== OTOI surface (pass-through to otoi::OTOIManager) =====================
    
    otoi::OtoiCharter parseCharter(const nlohmann::json& json);
    std::optional<otoi::OtoiCharter> safeParseCharter(const nlohmann::json& json, otoi::ASFDKError* error = nullptr);
    otoi::EffectivePolicy honor(const otoi::OtoiCharter& charter, const otoi::HonorOptions& options = {});
    nlohmann::json propagate(const otoi::EffectivePolicy& policy, const std::string& agentId);

    // ===================== RRT surface (pass-through to rrt::RRTAdvocate) =====================
    
    rrt::CrisisAssessment assessMessage(const std::string& message);

    // ===================== Sleepwalker surface (pass-through to sleepwalker::SleepwalkerProtocol) =====================
    
    sleepwalker::InteractionAssessment assessInteraction(
        const std::string& userInput,
        const std::vector<std::string>& sessionHistory = {},
        const std::optional<std::string>& userId = std::nullopt
    );
    void maintainContinuity(const std::string& userId, nlohmann::json sessionData);

    // ===================== Unified governance surface =====================
    
    GovernanceResult validateInteraction(const std::string& interactionId);
    GovernanceResult checkAgentWellness();
    GovernanceResult checkpointState();

    // ===================== Foundation integration =====================
    
    Envelope process(const std::string& input, const std::string& channel);
    AssessmentResult assess(const std::string& input);
    FoundationStatus getStatus();

private:
    // Actual pillar instances
    std::unique_ptr<toi::TOIManager> m_toi;
    std::unique_ptr<otoi::OTOIManager> m_otoi;
    std::unique_ptr<rrt::RRTAdvocate> m_rrt;
    std::unique_ptr<sleepwalker::SleepwalkerProtocol> m_sleepwalker;
};

} // namespace asfdk