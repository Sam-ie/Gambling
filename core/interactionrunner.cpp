#include "interactionrunner.h"
#include <chrono>

InteractionRunner::InteractionRunner(double errorRate)
    : m_errorRate(errorRate)
{
    m_rng.seed(static_cast<unsigned>(std::chrono::steady_clock::now()
                                     .time_since_epoch().count()));
}

int InteractionRunner::applyError(int intendedAction)
{
    if (m_errorRate <= 0.0) return intendedAction;
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    if (dist(m_rng) < m_errorRate)
        return std::uniform_int_distribution<int>(0, 1)(m_rng);
    return intendedAction;
}

InteractionRunner::Result
InteractionRunner::run(NPCBase& npcA, NPCBase& npcB,
                       HistoryRecord& history, const PayoffMatrix& payoff)
{
    int actionA = applyError(npcA.action(npcB.getId(), history));
    int actionB = applyError(npcB.action(npcA.getId(), history));

    history.recordInteraction(npcA.getId(), npcB.getId(), actionA, actionB);

    int scoreA, scoreB;
    payoff.calculate(actionA, actionB, scoreA, scoreB);
    npcA.addScore(scoreA);
    npcB.addScore(scoreB);

    npcA.postRoundUpdate(npcB.getId(), actionB, actionA, scoreA);
    npcB.postRoundUpdate(npcA.getId(), actionA, actionB, scoreB);

    return {actionA, actionB, scoreA, scoreB};
}

void InteractionRunner::runAllPairs(std::vector<std::unique_ptr<NPCBase>>& npcs,
                                     HistoryRecord& history,
                                     const PayoffMatrix& payoff)
{
    int n = static_cast<int>(npcs.size());
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            run(*npcs[i], *npcs[j], history, payoff);
        }
    }
}

InteractionRunner::Result
InteractionRunner::playerVsNpc(int playerAction, NPCBase& npc,
                                int playerId, HistoryRecord& history,
                                const PayoffMatrix& payoff)
{
    int npcAction = applyError(npc.action(playerId, history));

    history.recordInteraction(playerId, npc.getId(), playerAction, npcAction);

    int playerScoreChange, npcScoreChange;
    payoff.calculate(playerAction, npcAction, playerScoreChange, npcScoreChange);

    npc.addScore(npcScoreChange);
    npc.postRoundUpdate(playerId, playerAction, npcAction, npcScoreChange);

    return {playerAction, npcAction, playerScoreChange, npcScoreChange};
}
