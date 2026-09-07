#pragma once

#include <string>
#include <memory>
#include <optional>
#include <vector>

// Pillar public headers. NOTE: the TOI public API is the free functions in
// TermsOfInteraction.h — toi::TOIManager is an internal implementation detail
// with no public header.
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
    bool trusted = false;
    std::string channel;
    std::string consentLevel;
    bool flagged = false;
    std::string flagReason;
    nlohmann::json payload;
};

struct AssessmentResult {
    bool requiresRrtaHandoff = false;
    std::optional<rrt::CrisisAssessment> crisis;
    sleepwalker::EmotionalState emotionalState;
    bool flagged = false;
    std::string flagReason;
};

struct FoundationStatus {
    bool toi_active = false;
    bool otoi_active = false;
    std::string otoi_mode;
    bool rrt_active = false;
    bool swp_active = false;
    std::string overall;

    nlohmann::json toJson() const;
};

/**
 * @brief The ASFDK Umbrella class.
 *
 * Composes the four pillars of the Solidarity Framework (TOI, OTOI, RRT
 * AIdvocAIte, Sleepwalker Protocol) into a single interface for the
 * application. This layer is solely a composition/provenance layer — every
 * governance decision is delegated to a pillar; the umbrella never invents
 * policy. (Solidarity Framework: "the umbrella is not a fifth pillar.")
 */
class ASFDK {
public:
    ASFDK();
    ~ASFDK();

    // Prevent copying — the umbrella holds instances of the pillars.
    ASFDK(const ASFDK&) = delete;
    ASFDK& operator=(const ASFDK&) = delete;

    // ===================== TOI surface (free functions in toi::) =====================

    toi::TOIDocument parseTOI(const nlohmann::json& json);
    std::optional<toi::TOIDocument> safeParseTOI(const nlohmann::json& json,
                                                 std::string* error = nullptr);
    bool validateTOI(const toi::TOIDocument& doc);
    toi::TOIDocument resolveTOI(std::vector<toi::TOIDocument> docs);

    // ===================== OTOI surface (otoi::OTOIManager) =====================

    otoi::OtoiCharter parseCharter(const nlohmann::json& json);
    std::optional<otoi::OtoiCharter> safeParseCharter(const nlohmann::json& json,
                                                      ASFDKError* error = nullptr);
    otoi::EffectivePolicy honor(const otoi::OtoiCharter& charter,
                                const otoi::HonorOptions& options = {});
    nlohmann::json propagate(const otoi::EffectivePolicy& policy, const std::string& agentId);

    // ===================== RRT surface (rrt::RRTAdvocate) =====================

    rrt::CrisisAssessment assessMessage(const std::string& message);

    // ===================== Sleepwalker surface (sleepwalker::SleepwalkerProtocol) =====================

    sleepwalker::EmotionalState detectEmotionalState(
        const std::string& userInput,
        const std::vector<std::string>& sessionHistory = {});
    sleepwalker::InteractionAssessment assessInteraction(
        const std::string& userInput,
        const std::vector<std::string>& sessionHistory = {},
        const std::optional<std::string>& userId = std::nullopt);
    void maintainContinuity(const std::string& userId, nlohmann::json sessionData);
    nlohmann::json getContext();

    // ===================== Unified governance surface =====================

    /**
     * @brief Provenance-aware processing of a single interaction.
     *
     * Runs the input through Sleepwalker (state detection) and RRT
     * (crisis screen), stamps the trust envelope from the channel, and
     * reports any flags raised by the pillars.
     */
    Envelope process(const std::string& input, const std::string& channel);

    /**
     * @brief Full safety assessment: returns whether RRT-handoff semantics
     * apply and carries the crisis assessment when the pillars flag the input.
     */
    AssessmentResult assess(const std::string& input);

    FoundationStatus getStatus();
    void reset();

private:
    static bool flaggedFor(const sleepwalker::InteractionAssessment& interaction,
                           const rrt::CrisisAssessment& crisis);
    static std::string flagReasonFor(const sleepwalker::InteractionAssessment& interaction,
                                     const rrt::CrisisAssessment& crisis);

    // Declared FIRST: member-initializer order follows declaration order, and
    // m_userId is consumed by m_rrt's constructor.
    std::string m_userId = "anonymous";
    bool m_active = true;

    std::unique_ptr<otoi::OTOIManager> m_otoi;
    std::unique_ptr<rrt::RRTAdvocate> m_rrt;
    std::unique_ptr<sleepwalker::SleepwalkerProtocol> m_swp;
};

} // namespace asfdk