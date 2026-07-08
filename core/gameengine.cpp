#include "gameengine.h"
#include <algorithm>
#include <QDebug>

GameEngine::GameEngine(QObject* parent)
    : QObject(parent)
    , m_evoEngine(this)
{
    // 将子模块指针绑定到演化引擎
    m_evoEngine.bind(&m_npcs, &m_npcTypes, &m_history,
                     &m_runner, &m_payoff, &m_eliminator, &m_nextNpcId);

    // 转发演化引擎信号
    connect(&m_evoEngine, &AutoEvolutionEngine::step,
            this, &GameEngine::autoEvoStep);
    connect(&m_evoEngine, &AutoEvolutionEngine::pairDone,
            this, &GameEngine::autoEvoPairDone);
    connect(&m_evoEngine, &AutoEvolutionEngine::finished,
            this, &GameEngine::autoEvoFinished);
}

// ============ 初始化 ============

void GameEngine::initializeNPCs(const NPCConfig& config)
{
    m_npcs.clear();
    m_npcTypes.clear();
    m_nextNpcId = 0;

    config.forEach([this](NPCFactory::NPCType type, int count) {
        QString prefix = NPCFactory::defaultPrefix(type);
        for (int i = 0; i < count; ++i) {
            m_npcs.push_back(NPCFactory::create(type, m_nextNpcId++,
                prefix + "-" + QString::number(i + 1)));
            m_npcTypes.push_back(type);
        }
    });
    m_nextNpcId++; // 为玩家位预留
}

// ============ 游戏控制 ============

void GameEngine::startGame(int totalRounds)
{
    if (m_npcs.empty()) return;

    m_totalRounds = totalRounds;
    m_currentRound = 0;
    m_playerScore = 0;
    m_currentOpponentIdx = 0;

    for (auto& npc : m_npcs) npc->resetScore();

    int playerSlot = static_cast<int>(m_npcs.size());
    m_history.ensureSize(playerSlot + 1);

    m_phase = NPC_INTERACTION;
    emit gameStarted();
    emit roundChanged(1, m_totalRounds);
    runNPCRound();
}

void GameEngine::resetGame()
{
    stopAutoEvolution();
    m_npcs.clear();
    m_npcTypes.clear();
    m_history.clear();
    m_playerScore = 0;
    m_currentRound = 0;
    m_currentOpponentIdx = 0;
    m_phase = IDLE;
}

// ============ NPC 间交互 ============

void GameEngine::runNPCRound()
{
    m_runner.runAllPairs(m_npcs, m_history, m_payoff);

    emit npcInteractionsComplete();
    emit allScoresUpdated(buildRankings());

    m_currentOpponentIdx = 0;
    m_phase = WAITING_FOR_PLAYER;
    advanceTurn();
}

// ============ 玩家操作 ============

void GameEngine::playerAction(int action)
{
    if (m_phase != WAITING_FOR_PLAYER) return;
    if (m_currentOpponentIdx >= static_cast<int>(m_npcs.size())) return;

    int playerIdx = static_cast<int>(m_npcs.size());
    auto& npc = m_npcs[m_currentOpponentIdx];

    auto result = m_runner.playerVsNpc(action, *npc, playerIdx,
                                        m_history, m_payoff);
    m_playerScore += result.scoreA;

    emit turnResult(npc->getId(), npc->getName(),
                    action, result.actionB,
                    result.scoreA, result.scoreB);
    emit allScoresUpdated(buildRankings());

    m_currentOpponentIdx++;
    advanceTurn();
}

// ============ 回合推进 ============

void GameEngine::advanceTurn()
{
    if (m_currentOpponentIdx < static_cast<int>(m_npcs.size())) {
        const auto& npc = m_npcs[m_currentOpponentIdx];
        emit playerTurn(npc->getId(), npc->getName(), npc->getStrategyType());
        return;
    }

    // 所有对手交互完毕
    m_currentRound++;
    if (m_currentRound >= m_totalRounds) {
        m_phase = FINISHED;
        emit allScoresUpdated(buildRankings());
        emit gameFinished(m_playerScore);
        return;
    }

    applyEliminationIfDue();

    emit roundChanged(m_currentRound + 1, m_totalRounds);
    m_phase = NPC_INTERACTION;
    runNPCRound();
}

void GameEngine::applyEliminationIfDue()
{
    if (m_eliminator.shouldEliminate(m_currentRound)) {
        m_eliminator.apply(m_npcs, m_npcTypes, m_nextNpcId, m_history);
        m_eliminator.adjustScores(m_npcs, nullptr);
        m_playerScore = m_eliminator.adjustPlayerScore(m_playerScore);
    }
}

// ============ 配置 ============

void GameEngine::setErrorRate(double rate)
{
    m_runner.setErrorRate(rate);
    m_evoEngine.setErrorRate(rate);
}

void GameEngine::setScoreInheritRatio(int pct)
{
    m_eliminator.setScoreInheritRatio(pct / 100.0);
}

int GameEngine::scoreInheritRatio() const
{
    return static_cast<int>(m_eliminator.scoreInheritRatio() * 100);
}

void GameEngine::setAutoEvolutionSpeed(int ms)
{
    m_evoEngine.setAutoEvoInterval(ms);
}

// ============ 查询 ============

QVector<NPCScoreInfo> GameEngine::buildRankings() const
{
    QVector<NPCScoreInfo> rankings;
    rankings.reserve(static_cast<int>(m_npcs.size()));
    for (const auto& npc : m_npcs) {
        rankings.append({npc->getId(), npc->getName(),
                         npc->getStrategyType(), npc->getScore()});
    }
    std::sort(rankings.begin(), rankings.end(),
              [](const NPCScoreInfo& a, const NPCScoreInfo& b) {
                  return a.score > b.score;
              });
    return rankings;
}

QVector<InteractionPair> GameEngine::getPlayerInteractionHistory(int npcId) const
{
    int playerSlot = static_cast<int>(m_npcs.size());
    return m_history.getInteractionHistory(playerSlot, npcId);
}

// ============ 作弊 ============

void GameEngine::cheatAddScore(int points)
{
    m_playerScore += points;
    emit allScoresUpdated(buildRankings());
}

void GameEngine::setNPCScores(const QVector<QPair<int, int>>& updates)
{
    for (const auto& upd : updates) {
        int idx = upd.first;
        if (idx >= 0 && idx < static_cast<int>(m_npcs.size())) {
            m_npcs[idx]->setScore(upd.second);
        }
    }
    emit allScoresUpdated(buildRankings());
}

// ============ 自动演化委托 ============

void GameEngine::startAutoEvolution(int totalRounds, bool startTimer)
{
    if (m_npcs.empty()) return;

    m_totalRounds = totalRounds;
    m_currentRound = 0;
    m_playerScore = 0;

    for (auto& npc : m_npcs) npc->resetScore();
    m_history.ensureSize(static_cast<int>(m_npcs.size()));
    m_history.clear();

    m_evoEngine.start(totalRounds, startTimer);
}

void GameEngine::startAutoEvolutionFast()
{
    if (!m_evoEngine.isRunning()) {
        startAutoEvolution(m_totalRounds, true);
    }
    m_evoEngine.fastMode();
}
