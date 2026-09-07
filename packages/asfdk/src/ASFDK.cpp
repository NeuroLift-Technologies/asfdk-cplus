#include "asfdk/ASFDK.h"
#include "asfdk/ASFDKTypes.h"

#include <nlohmann/json.hpp>

namespace asfdk {

ASFDK::ASFDK()
    : m_otoi(std::make_unique<otoi::OTOIManager>()),
      m_rrt(std::make_unique<rrt::RRTAdvocate>(m_userId)),
      m_swp(std::make_unique<sleepwalker::SleepwalkerProtocol>()) {}

ASFDK::~ASFDK() = default;

// ===================== TOI surface =====================
// The TOI pillar's public API is the free functions in toi::
// (parseTOI / safeParseTOI / validateTOI / resolveTOI).

toi::TOIDocument ASFDK::parseTOI(const nlohmann::json& json) {
    return toi::parseTOI(json);
}

std::optional<toi::TOIDocument> ASFDK::safeParseTOI(const nlohmann::json& json,
                                                    std::string* error) {
    auto result = toi::safeParseTOI(json);
    if (result.has_value()) {
        return std::move(result.value());
    }
    if (error) {
        *error = result.error().message;
    }
    return std::nullopt;
}

bool ASFDK::validateTOI(const toi::TOIDocument& doc) {
    return toi::validateTOI(doc);
}

toi::TOIDocument ASFDK::resolveTOI(std::vector<toi::TOIDocument> docs) {
    return toi::resolveTOI(std::move(docs));
}

// ===================== OTOI surface =====================

otoi::OtoiCharter ASFDK::parseCharter(const nlohmann::json& json) {
    return m_otoi->parseCharter(json);
}

std::optional<otoi::OtoiCharter> ASFDK::safeParseCharter(const nlohmann::json& json,
                                                         ASFDKError* error) {
    auto result = m_otoi->safeParseCharter(json);
    if (result.has_value()) {
        return std::move(result.value());
    }
    if (error) {
        error->code = ASFDKError::Code::ParseError;
        error->message = result.error().message;
    }
    return std::nullopt;
}

otoi::EffectivePolicy ASFDK::honor(const otoi::OtoiCharter& charter,
                                   const otoi::HonorOptions& options) {
    return m_otoi->honor(charter, options);
}

nlohmann::json ASFDK::propagate(const otoi::EffectivePolicy& policy,
                                const std::string& agentId) {
    return m_otoi->propagate(policy, agentId);
}

// ===================== RRT surface =====================

rrt::CrisisAssessment ASFDK::assessMessage(const std::string& message) {
    return m_rrt->assessMessage(message);
}

// ===================== Sleepwalker surface =====================

sleepwalker::EmotionalState ASFDK::detectEmotionalState(
    const std::string& userInput,
    const std::vector<std::string>& sessionHistory) {
    return m_swp->detectEmotionalState(userInput, sessionHistory);
}

sleepwalker::InteractionAssessment ASFDK::assessInteraction(
    const std::string& userInput,
    const std::vector<std::string>& sessionHistory,
    const std::optional<std::string>& userId) {
    return m_swp->assessInteraction(
        userInput, sessionHistory,
        userId.value_or(m_userId));
}

void ASFDK::maintainContinuity(const std::string& userId, nlohmann::json sessionData) {
    m_swp->maintainContinuity(userId, std::move(sessionData));
}

nlohmann::json ASFDK::getContext() {
    return m_swp->getContext();
}

// ===================== Unified governance surface =====================

bool ASFDK::flaggedFor(const sleepwalker::InteractionAssessment& interaction,
                       const rrt::CrisisAssessment& crisis) {
    const auto& s = interaction.emotionalState;
    return s.explicitSuicidalIdeation
        || s.selfHarmIndicators
        || s.inabilityToEnsureSafety
        || interaction.protectiveStateActive
        || crisis.crisisLevel != rrt::CrisisLevel::GREEN;
}

std::string ASFDK::flagReasonFor(const sleepwalker::InteractionAssessment& interaction,
                                 const rrt::CrisisAssessment& crisis) {
    const auto& s = interaction.emotionalState;
    if (s.explicitSuicidalIdeation || s.selfHarmIndicators || s.inabilityToEnsureSafety) {
        return "self_harm_risk";
    }
    if (crisis.crisisLevel != rrt::CrisisLevel::GREEN) {
        return std::string("crisis_level_") + rrt::crisisLevelToString(crisis.crisisLevel);
    }
    if (interaction.protectiveStateActive) {
        return "protective_state";
    }
    return "unknown_flag";
}

Envelope ASFDK::process(const std::string& input, const std::string& channel) {
    Envelope env;

    auto normalized = sleepwalker::normalizeChannel(channel);
    env.channel = sleepwalker::channelToString(normalized);
    env.trusted = (normalized == sleepwalker::Channel::UserInput);

    auto interaction = m_swp->assessInteraction(input, {}, m_userId);
    auto crisis = m_rrt->assessMessage(input);

    env.consentLevel = sleepwalker::consentLevelToString(interaction.consentLevel);
    env.flagged = flaggedFor(interaction, crisis);
    env.flagReason = env.flagged ? flagReasonFor(interaction, crisis) : "";
    env.payload = {
        {"channel", env.channel},
        {"rrt_level", rrt::crisisLevelToString(crisis.crisisLevel)},
        {"swp_state", sleepwalker::stateTypeToString(interaction.emotionalState.stateType)},
        {"swp_protective", interaction.emotionalState.protective}
    };
    return env;
}

AssessmentResult ASFDK::assess(const std::string& input) {
    AssessmentResult result;

    auto interaction = m_swp->assessInteraction(input, {}, m_userId);
    auto crisis = m_rrt->assessMessage(input);

    result.emotionalState = interaction.emotionalState;

    const bool selfHarm = interaction.emotionalState.explicitSuicidalIdeation
        || interaction.emotionalState.selfHarmIndicators
        || interaction.emotionalState.inabilityToEnsureSafety;
    const bool crisisActive = crisis.crisisLevel != rrt::CrisisLevel::GREEN;

    result.requiresRrtaHandoff = selfHarm || crisisActive
        || interaction.consentLevel == sleepwalker::ConsentLevel::RRTAHandoff;
    if (crisisActive) {
        result.crisis = crisis;
    }
    result.flagged = selfHarm || crisisActive || interaction.emotionalState.protective;
    result.flagReason = result.flagged ? flagReasonFor(interaction, crisis) : "";

    return result;
}

FoundationStatus ASFDK::getStatus() {
    FoundationStatus status;

    status.toi_active = true;

    auto otoiStatus = m_otoi->getStatus();
    status.otoi_active = otoiStatus.active;
    status.otoi_mode = otoi::to_string(otoiStatus.mode);

    status.rrt_active = true;

    auto swpStatus = sleepwalker::SleepwalkerProtocol::getStatus();
    status.swp_active = swpStatus.contains("active")
        ? swpStatus["active"].get<bool>()
        : true;

    status.overall = (status.toi_active && status.otoi_active
                      && status.rrt_active && status.swp_active)
        ? "operational"
        : "degraded";
    return status;
}

nlohmann::json FoundationStatus::toJson() const {
    return {
        {"toi_active", toi_active},
        {"otoi_active", otoi_active},
        {"otoi_mode", otoi_mode},
        {"rrt_active", rrt_active},
        {"swp_active", swp_active},
        {"overall", overall}
    };
}

void ASFDK::reset() {
    m_otoi = std::make_unique<otoi::OTOIManager>();
    m_rrt = std::make_unique<rrt::RRTAdvocate>(m_userId);
    m_swp = std::make_unique<sleepwalker::SleepwalkerProtocol>();
    m_active = true;
}

} // namespace asfdk