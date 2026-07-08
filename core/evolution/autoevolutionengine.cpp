#include "autoevolutionengine.h"
#include "evoidle.h"
#include "evopairbypair.h"
#include "evofast.h"
#include "evosingleround.h"
#include <algorithm>

AutoEvolutionEngine::AutoEvolutionEngine(QObject* parent)
    : QObject(parent)
    , m_state(std::make_unique<EvoIdle>())
{
}

AutoEvolutionEngine::~AutoEvolutionEngine() = default;

void AutoEvolutionEngine::bind(
    std::vector<std::unique_ptr<NPCBase>>* npcs,
    std::vector<NPCFactory::NPCType>* types,
    HistoryRecord* history,
    InteractionRunner* runner,
    PayoffMatrix* payoff,
    EliminationManager* eliminator,
    int* nextNpcId)
{
    m_npcs = npcs;
    m_npcTypes = types;
    m_history = history;
    m_runner = runner;
    m_payoff = payoff;
    m_eliminator = eliminator;
    m_nextNpcId = nextNpcId;
}

void AutoEvolutionEngine::transitionTo(EvolutionState* newState)
{
    m_state.reset(newState);
    m_state->enter(this);
}

// ---- 生命周期 ----

void AutoEvolutionEngine::start(int totalRounds, bool startTimer)
{
    stop();

    m_targetRound = totalRounds;
    m_currentRound = 0;
    m_pairI = 0;
    m_pairJ = 1;
    m_running = true;
    m_paused = false;

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, [this]() {
        if (m_state) m_state->tick(this);
    });

    if (startTimer) {
        transitionTo(new EvoPairByPair());
        m_timer->start(m_timerInterval);
    } else {
        transitionTo(new EvoPairByPair());
        m_paused = true;
    }

    emit step(0, totalRounds, buildRankings());
}

void AutoEvolutionEngine::stop()
{
    if (m_timer) {
        m_timer->stop();
        delete m_timer;
        m_timer = nullptr;
    }
    m_running = false;
    m_paused = false;
    transitionTo(new EvoIdle());
}

void AutoEvolutionEngine::pause()
{
    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
        m_paused = true;
    }
}

void AutoEvolutionEngine::resume()
{
    if (m_paused && m_timer) {
        m_paused = false;
        m_timer->start(m_timerInterval);
    }
}

void AutoEvolutionEngine::stepPair()
{
    if (!m_running) return;
    if (m_currentRound >= m_targetRound) return;

    int n = static_cast<int>(m_npcs->size());
    if (n < 2) return;

    // 手动触发一次 tick
    if (m_state) m_state->tick(this);
}

void AutoEvolutionEngine::stepRound()
{
    if (!m_running) return;
    if (m_currentRound >= m_targetRound) return;

    // 切换到单轮模式
    transitionTo(new EvoSingleRound());
    if (m_timer && !m_timer->isActive()) {
        m_timer->start(m_timerInterval);
    }
}

void AutoEvolutionEngine::fastMode()
{
    if (!m_running) return;

    transitionTo(new EvoFast());
    m_timer->setInterval(50);
    if (!m_timer->isActive()) {
        m_timer->start(50);
    }
}

// ---- 回合推进 ----

void AutoEvolutionEngine::advanceRound()
{
    m_currentRound++;

    // 检查是否完成
    if (m_currentRound >= m_targetRound) {
        stop();
        emit finished(buildRankings());
        return;
    }

    // 淘汰 & 折算
    if (m_eliminator->shouldEliminate(m_currentRound)) {
        m_eliminator->apply(*m_npcs, *m_npcTypes, *m_nextNpcId, *m_history);
        m_eliminator->adjustScores(*m_npcs, nullptr);
    }

    // 重置配对索引，开始下一轮
    m_pairI = 0;
    m_pairJ = 1;
}

void AutoEvolutionEngine::emitStep()
{
    emit step(m_currentRound + 1, m_targetRound, buildRankings());
}

void AutoEvolutionEngine::emitPairDone(int a, int b)
{
    emit pairDone(a, b);
}

QVector<NPCScoreInfo> AutoEvolutionEngine::buildRankings() const
{
    QVector<NPCScoreInfo> rankings;
    rankings.reserve(static_cast<int>(m_npcs->size()));
    for (const auto& npc : *m_npcs) {
        rankings.append({npc->getId(), npc->getName(),
                         npc->getStrategyType(), npc->getScore()});
    }
    std::sort(rankings.begin(), rankings.end(),
              [](const NPCScoreInfo& a, const NPCScoreInfo& b) {
                  return a.score > b.score;
              });
    return rankings;
}
