#include "asfdk/ASFDK.h"
#include "asfdk/ASFDKTypes.h"

#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace asfdk {

ASFDK::ASFDK()
    : m_toi(std::make_unique<toi::TOIManager>())
    , m_otoi(std::make_unique<otoi::OTOIManager>())
    , m_rrt(std::make_unique<rrt::RRTAdvocate>())
    , m_sleepwalker(std::make_unique<sleepwalker::SleepwalkerProtocol>())
{
}

ASFDK::~ASFDK() = default;

// ===================== TOI surface =====================

toi::TOIDocument ASFDK::parseTOI(const nlohmann::json& json) {
    return m_toi->parseTOI(json);
}

std::optional<toi::TOIDocument> ASFDK::safeParseTOI(const nlohmann::json& json, std::string* error) {
    return m_toi->safeParseTOI(json, error);
}

bool ASFDK::validateTOI(const toi::TOIDocument& doc) {
    return m_toi->validateTOI(doc);
}

toi::TOIDocument ASFDK::resolveTOI(std::vector<toi::TOIDocument> docs) {
    return m_toi->resolveTOI(docs);
}

std::string ASFDK::canonicalize(const std::string& json) {
    return m_toi->canonicalize(json);
}

// ===================== OTOI surface =====================

otoi::OtoiCharter ASFDK::parseCharter(const nlohmann::json& json) {
    return m_otoi->parseCharter(json);
}

std::optional<otoi::OtoiCharter> ASFDK::safeParseCharter(const nlohmann::json& json, otoi::ASFDKError* error) {
    return m_otoi->safeParseCharter(json, error);
}

otoi::EffectivePolicy ASFDK::honor(const otoi::OtoiCharter& charter, const otoi::HonorOptions& options) {
    return m_otoi->honor(charter, options);
}

nlohmann::json ASFDK::propagate(const otoi::EffectivePolicy& policy, const std::string& agentId) const {
    return m_otoi->propagate(policy, agentId);
}

// ===================== RRT surface =====================

rrt::CrisisAssessment ASFDK::assessMessage(const std::string& message) {
    return m_rrt->assessMessage(message);
}

rrt::CrisisAssessment ASFDK::assessCurrentState() {
    return m_rrt->assessCurrentState();
}

rrt::BurnoutAssessment ASFDK::assessBurnout(const rrt::SessionMetrics& metrics) {
    return m_rrt->assessBurnout(metrics);
}

nlohmann::json ASFDK::getStatusReport() const {
    return m_rrt->getStatusReport();
}

// ===================== Sleepwalker surface =====================

sleepwalker::EmotionalState ASFDK::detectEmotionalState(
    const std::string& userInput,
    const std::vector<std::string>& sessionHistory = {}
) {
    return m_sleepwalker->detectEmotionalState(userInput, sessionHistory);
}

sleepwalker::InteractionAssessment ASFDK::assessInteraction(
    const std::string& userInput,
    const std::vector<std::string>& sessionHistory = {},
    const std::optional<std::string>& userId = std::nullopt
) {
    return m_sleepwalker->assessInteraction(userInput, sessionHistory, userId);
}

sleepwalker::ConsentLevel ASFDK::determineAppropriateLevel(const sleepwalker::EmotionalState& state) {
    return m_sleepwalker->determineAppropriateLevel(state);
}

bool ASFDK::requiresRrtaHandoff(const sleepwalker::EmotionalState& state) {
    return m_sleepwalker->requiresRrtaHandoff(state);
}

nlohmann::json ASFDK::getContext() {
    return m_sleepwalker->getContext();
}

void ASFDK::maintainContinuity(const std::string& userId, nlohmann::json sessionData) {
    m_sleepwalker->maintainContinuity(userId, sessionData);
}

nlohmann::json ASFDK::getStatus() {
    return sleepwalker::SleepwalkerProtocol::getStatus();
}

void ASFDK::reset() {
    sleepwalker::SleepwalkerProtocol::reset();
}

// ===================== Unified governance surface =====================

GovernanceResult ASFDK::validateInteraction(const std::string& interactionId) {
    try {
        // Create a minimal TOI document for validation
        nlohmann::json interactionJson = {
            {"$toi", "1.0.0"},
            {"$tier", "personal"},
            {"identity", {{"author", interactionId}}}
        };
        
        auto result = m_toi->safeParseTOI(interactionJson);
        if (!result.has_value()) {
            return {false, "TOI validation failed: " + result.error().message, static_cast<int>(result.error().code)};
        }
        
        // Check OTOI governance status
        auto status = m_otoi->getStatus();
        if (!status.active) {
            return {false, "Governance system inactive", 1};
        }
        
        return {true, "Interaction validated successfully", 0};
    } catch (const std::exception& e) {
        return {false, std::string("Exception during validation: ") + e.what(), -1};
    }
}

GovernanceResult ASFDK::checkAgentWellness() {
    try {
        // Use RRT Advocate to assess current state
        auto crisisAssessment = m_rrt->assessCurrentState();
        auto burnoutAssessment = m_rrt->assessBurnout({
            0,  // messagesProcessed
            0,  // crisisTriggers
            0,  // interventionCount
            0.0,  // avgCrisisScore
            0.0,  // sessionDurationMinutes
            std::chrono::system_clock::now()
        });

        if (crisisAssessment.severity >= rrt::CrisisSeverity::High) {
            return {false, "Crisis detected: " + crisisAssessment.primaryConcern, 1};
        }
        
        if (burnoutAssessment.level >= rrt::BurnoutLevel::High) {
            return {false, "Burnout detected: " + burnoutAssessment.primaryIndicator, 2};
        }

        return {true, "Agent wellness check passed", 0};
    } catch (const std::exception& e) {
        return {false, std::string("Exception during wellness check: ") + e.what(), -1};
    }
}

GovernanceResult ASFDK::checkpointState() {
    try {
        // Use Sleepwalker to maintain continuity
        nlohmann::json sessionData = {
            {"timestamp", std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()},
            {"pillars_active", {"toi", "otoi", "rrt", "sleepwalker"}}
        };
        
        m_sleepwalker->maintainContinuity("asfdk-session", sessionData);
        
        return {true, "State checkpoint saved", 0};
    } catch (const std::exception& e) {
        return {false, std::string("Exception during checkpoint: ") + e.what(), -1};
    }
}

} // namespace asfdk