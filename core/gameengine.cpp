#include "gameengine.h"
#include <algorithm>
#include <numeric>
#include <cstdlib>
#include <ctime>
#include <random>
#include <limits>
#include <QDebug>

GameEngine::GameEngine(QObject* parent)
    : QObject(parent)
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));
}

GameEngine::~GameEngine() = default;

// ============ 初始化 ============

void GameEngine::initializeNPCs(int honestCount, int deceptiveCount,
                                int swingerCount, int repeaterCount,
                                int forgivingCount, int reinforcementCount)
{
    m_npcs.clear();
    m_npcTypes.clear();
    m_nextNpcId = 0;

    auto createBatch = [&](NPCFactory::NPCType type, int count, const QString& prefix) {
        for (int i = 0; i < count; ++i) {
            m_npcs.push_back(std::unique_ptr<NPCBase>(
                NPCFactory::createNPC(type, m_nextNpcId++, prefix + "-" + QString::number(i + 1))));
            m_npcTypes.push_back(type);
        }
    };

    createBatch(NPCFactory::HONEST,        honestCount,      "诚实者");
    createBatch(NPCFactory::DECEPTIVE,     deceptiveCount,   "背叛者");
    createBatch(NPCFactory::SWINGER,       swingerCount,     "摇摆者");
    createBatch(NPCFactory::REPEATER,      repeaterCount,    "复读者");
    createBatch(NPCFactory::FORGIVING,     forgivingCount,   "宽恕者");
    createBatch(NPCFactory::REINFORCEMENT, reinforcementCount, "强化学习");
    m_nextNpcId++; // 为玩家位留空，避免 NPC 克隆 ID 冲突
}

// ============ 游戏控制 ============

void GameEngine::startGame(int totalRounds)
{
    if (m_npcs.empty()) return;

    m_totalRounds = totalRounds;
    m_currentRound = 0;
    m_playerScore = 0;
    m_currentOpponentIdx = 0;

    for (auto& npc : m_npcs) {
        npc->resetScore();
    }

    // 历史记录: npcCount 个 NPC + 1 个玩家槽位
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
    int n = static_cast<int>(m_npcs.size());
    bool hasError = (m_errorRate > 0.0);
    double errorThreshold = m_errorRate * RAND_MAX;

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            int actionI = m_npcs[i]->action(m_npcs[j]->getId(), m_history);
            int actionJ = m_npcs[j]->action(m_npcs[i]->getId(), m_history);

            // 失误率：NPC 有一定概率随机选择
            if (hasError && std::rand() < errorThreshold) actionI = std::rand() % 2;
            if (hasError && std::rand() < errorThreshold) actionJ = std::rand() % 2;

            m_history.recordInteraction(m_npcs[i]->getId(), m_npcs[j]->getId(),
                                        actionI, actionJ);

            int scoreI, scoreJ;
            calculateScores(actionI, actionJ, scoreI, scoreJ);
            m_npcs[i]->addScore(scoreI);
            m_npcs[j]->addScore(scoreJ);

            // 强化学习 NPC 更新
            m_npcs[i]->postRoundUpdate(m_npcs[j]->getId(), actionJ, actionI, scoreI);
            m_npcs[j]->postRoundUpdate(m_npcs[i]->getId(), actionI, actionJ, scoreJ);
        }
    }

    emit npcInteractionsComplete();
    emit allScoresUpdated(buildRankings());

    // 进入玩家回合
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

    int npcAction = npc->action(playerIdx, m_history);

    // 失误率：对手 NPC 有概率随机选择（玩家不受影响）
    if (m_errorRate > 0.0 && std::rand() < m_errorRate * RAND_MAX) {
        npcAction = std::rand() % 2;
    }

    m_history.recordInteraction(playerIdx, npc->getId(), action, npcAction);

    int playerScoreChange, npcScoreChange;
    calculateScores(action, npcAction, playerScoreChange, npcScoreChange);
    m_playerScore += playerScoreChange;
    npc->addScore(npcScoreChange);

    npc->postRoundUpdate(playerIdx, action, npcAction, npcScoreChange);

    emit turnResult(npc->getId(), npc->getName(),
                    action, npcAction,
                    playerScoreChange, npcScoreChange);
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

    // 所有对手交互完毕，推进回合
    m_currentRound++;
    if (m_currentRound >= m_totalRounds) {
        m_phase = FINISHED;
        emit allScoresUpdated(buildRankings());
        emit gameFinished(m_playerScore);
        return;
    }

    // 回合结束：应用淘汰/遗传
    if (m_eliminationInterval > 0 && m_currentRound % m_eliminationInterval == 0) {
        applyElimination();
    }

    emit roundChanged(m_currentRound + 1, m_totalRounds);
    m_phase = NPC_INTERACTION;
    runNPCRound();
}

// ============ 辅助方法 ============

void GameEngine::calculateScores(int action1, int action2,
                                  int& score1, int& score2)
{
    // 使用成员变量，可配置的支付矩阵
    if (action1 == 0 && action2 == 0) {
        score1 = m_cooperateReward; score2 = m_cooperateReward;
    } else if (action1 == 0 && action2 == 1) {
        score1 = m_betrayedPenalty; score2 = m_cheatReward;
    } else if (action1 == 1 && action2 == 0) {
        score1 = m_cheatReward; score2 = m_betrayedPenalty;
    } else {
        score1 = m_bothCheatPenalty; score2 = m_bothCheatPenalty;
    }
}

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

void GameEngine::setNPCScores(const QVector<QPair<int, int>>& npcScoreUpdates)
{
    for (const auto& upd : npcScoreUpdates) {
        int idx = upd.first;
        int score = upd.second;
        if (idx >= 0 && idx < static_cast<int>(m_npcs.size())) {
            m_npcs[idx]->setScore(score);
        }
    }
    emit allScoresUpdated(buildRankings());
}

// ============ 淘汰机制 ============

void GameEngine::applyElimination()
{
    int n = static_cast<int>(m_npcs.size());
    if (n < 2) return;

    int eliminateCount = qMin(m_eliminationCount, n - 1);
    if (eliminateCount <= 0) return;

    // 找最高分（随机打破平局）
    int maxScore = std::numeric_limits<int>::min();
    for (auto& npc : m_npcs) maxScore = qMax(maxScore, npc->getScore());
    QVector<int> bestPool;
    for (int i = 0; i < n; ++i)
        if (m_npcs[i]->getScore() == maxScore) bestPool.append(i);
    int bestIdx = bestPool[std::rand() % bestPool.size()];
    NPCFactory::NPCType bestType = m_npcTypes[bestIdx];
    int parentScore = m_npcs[bestIdx]->getScore();

    // 找最低分淘汰对象（随机打破平局）
    QVector<int> sortedByScore(n);
    std::iota(sortedByScore.begin(), sortedByScore.end(), 0);
    std::sort(sortedByScore.begin(), sortedByScore.end(), [this](int a, int b) {
        return m_npcs[a]->getScore() < m_npcs[b]->getScore();
    });

    QSet<int> eliminated;
    for (int k = 0; k < eliminateCount; ++k) {
        // 收集所有未被淘汰的当前最低分者
        int curMinScore = std::numeric_limits<int>::max();
        for (int i = 0; i < n; ++i) {
            if (eliminated.contains(i)) continue;
            curMinScore = qMin(curMinScore, m_npcs[i]->getScore());
        }
        QVector<int> pool;
        for (int i = 0; i < n; ++i) {
            if (eliminated.contains(i)) continue;
            if (m_npcs[i]->getScore() == curMinScore) pool.append(i);
        }
        int worstIdx = pool[std::rand() % pool.size()];
        eliminated.insert(worstIdx);

        if (curMinScore >= parentScore) break; // 全部分数相同，停止

        // 克隆最高分 NPC
        int newId = m_nextNpcId++;
        QString newName = NPCFactory::getTypeName(bestType)
                        + QString("-c%1").arg(newId);
        m_npcs[worstIdx] = std::unique_ptr<NPCBase>(
            NPCFactory::createNPC(bestType, newId, newName));
        m_npcs[worstIdx]->setScore(parentScore);
        m_npcTypes[worstIdx] = bestType;
    }

    qDebug() << "淘汰: 删除了" << eliminated.size() << "个最低分NPC, 替换为"
             << NPCFactory::getTypeName(bestType);
}

void GameEngine::startAutoEvolution(int totalRounds, int honestCount,
                                     int deceptiveCount, int swingerCount,
                                     int repeaterCount, int forgivingCount,
                                     int reinforcementCount, bool startTimer)
{
    stopAutoEvolution();

    // 初始化 NPC（无玩家参与）
    initializeNPCs(honestCount, deceptiveCount, swingerCount,
                    repeaterCount, forgivingCount, reinforcementCount);

    for (auto& npc : m_npcs) {
        npc->resetScore();
    }

    m_history.ensureSize(static_cast<int>(m_npcs.size()));
    m_history.clear();

    m_currentRound = 0;
    m_autoEvoTargetRound = totalRounds;
    m_autoEvoPairI = 0;
    m_autoEvoPairJ = 1;
    m_autoEvoAdjustingRound = false;
    m_autoEvoWaitingAdjust = false;
    m_autoEvoFastMode = false;
    m_autoEvoSingleRound = false;
    m_playerScore = 0;

    // 创建定时器
    m_autoEvoTimer = new QTimer(this);
    connect(m_autoEvoTimer, &QTimer::timeout, this, &GameEngine::autoEvoTick);
    if (startTimer) {
        m_autoEvoTimer->start(m_autoEvoInterval);
    } else {
        m_autoEvoPaused = true;
    }

    emit autoEvoStep(0, totalRounds, buildRankings());
}

void GameEngine::stopAutoEvolution()
{
    if (m_autoEvoTimer) {
        m_autoEvoTimer->stop();
        delete m_autoEvoTimer;
        m_autoEvoTimer = nullptr;
    }
    m_autoEvoPaused = false;
}

void GameEngine::pauseAutoEvolution()
{
    if (m_autoEvoTimer && m_autoEvoTimer->isActive()) {
        m_autoEvoTimer->stop();
        m_autoEvoPaused = true;
    }
}

void GameEngine::resumeAutoEvolution()
{
    if (m_autoEvoPaused && m_autoEvoTimer) {
        m_autoEvoPaused = false;
        m_autoEvoTimer->start(m_autoEvoInterval);
    } else {
        m_autoEvoPaused = true;
    }
}

void GameEngine::startAutoEvolutionFast()
{
    m_autoEvoFastMode = true;
    m_autoEvoSingleRound = false;
    m_autoEvoTimer->setInterval(100);
    m_autoEvoTimer->start(100);
}

void GameEngine::stepAutoEvolution()
{
    // 单轮：逐对回显，定时器驱动一轮后自动停止
    if (!m_autoEvoTimer) return;
    if (m_currentRound >= m_autoEvoTargetRound) return;
    m_autoEvoFastMode = false;
    m_autoEvoSingleRound = true;
    m_autoEvoPaused = true;
    m_autoEvoTimer->setInterval(100);
    m_autoEvoTimer->start(100);
}

void GameEngine::stepAutoEvolutionPair()
{
    if (m_currentRound >= m_autoEvoTargetRound) return;

    int n = static_cast<int>(m_npcs.size());
    if (n < 2) return;

    // 步骤 A: 淘汰+折算步（仅淘汰回合触发）
    if (m_autoEvoAdjustingRound) {
        m_autoEvoAdjustingRound = false;
        m_currentRound++;
        if (m_currentRound >= m_autoEvoTargetRound) {
            emit autoEvoFinished(buildRankings());
            return;
        }
        applyElimination();
        adjustScores();
        m_autoEvoPairI = 0;
        m_autoEvoPairJ = 1;
        emit autoEvoStep(m_currentRound, m_autoEvoTargetRound, buildRankings());
        return;
    }

    // 步骤 B: 找下一对未交互的 (i, j)
    while (m_autoEvoPairI < n && m_autoEvoPairJ >= n) {
        m_autoEvoPairI++;
        m_autoEvoPairJ = m_autoEvoPairI + 1;
    }
    if (m_autoEvoPairI >= n || m_autoEvoPairJ >= n) {
        // 所有对已完成
        bool elimDue = (m_eliminationInterval > 0 && (m_currentRound + 1) % m_eliminationInterval == 0);
        if (elimDue) {
            // 淘汰回合：插入折算步
            m_autoEvoAdjustingRound = true;
            emit autoEvoStep(m_currentRound + 1, m_autoEvoTargetRound, buildRankings());
            emit autoEvoPairDone(-1, -1);
        } else {
            // 非淘汰回合：直接推进
            m_currentRound++;
            if (m_currentRound >= m_autoEvoTargetRound) {
                emit autoEvoFinished(buildRankings());
                return;
            }
            m_autoEvoPairI = 0;
            m_autoEvoPairJ = 1;
            emit autoEvoStep(m_currentRound, m_autoEvoTargetRound, buildRankings());
        }
        return;
    }

    int i = m_autoEvoPairI;
    int j = m_autoEvoPairJ;

    // 执行这一对
    bool hasError = (m_errorRate > 0.0);
    double errorThreshold = m_errorRate * RAND_MAX;

    int actionI = m_npcs[i]->action(m_npcs[j]->getId(), m_history);
    int actionJ = m_npcs[j]->action(m_npcs[i]->getId(), m_history);

    if (hasError && std::rand() < errorThreshold) actionI = std::rand() % 2;
    if (hasError && std::rand() < errorThreshold) actionJ = std::rand() % 2;

    m_history.recordInteraction(m_npcs[i]->getId(), m_npcs[j]->getId(),
                                actionI, actionJ);

    int scoreI, scoreJ;
    calculateScores(actionI, actionJ, scoreI, scoreJ);
    m_npcs[i]->addScore(scoreI);
    m_npcs[j]->addScore(scoreJ);
    m_npcs[i]->postRoundUpdate(m_npcs[j]->getId(), actionJ, actionI, scoreI);
    m_npcs[j]->postRoundUpdate(m_npcs[i]->getId(), actionI, actionJ, scoreJ);

    // 推进到下一对
    m_autoEvoPairJ++;
    if (m_autoEvoPairJ >= n) {
        m_autoEvoPairI++;
        m_autoEvoPairJ = m_autoEvoPairI + 1;
    }

    emit autoEvoStep(m_currentRound + 1, m_autoEvoTargetRound, buildRankings());
    emit autoEvoPairDone(m_npcs[i]->getId(), m_npcs[j]->getId());
}

void GameEngine::adjustScores()
{
    if (m_scoreInheritRatio >= 1.0) return;
    for (auto& npc : m_npcs) {
        int oldScore = npc->getScore();
        int newScore = static_cast<int>(static_cast<double>(oldScore) * m_scoreInheritRatio);
        npc->setScore(newScore);
    }
}

void GameEngine::autoEvoTick()
{
    // ====== 极速演化（一轮一帧） ======
    if (m_autoEvoFastMode) {
        int n = static_cast<int>(m_npcs.size());
        bool hasError = (m_errorRate > 0.0);
        double errorThreshold = m_errorRate * RAND_MAX;
        for (int i = 0; i < n; ++i)
            for (int j = i + 1; j < n; ++j) {
                int aI = m_npcs[i]->action(m_npcs[j]->getId(), m_history);
                int aJ = m_npcs[j]->action(m_npcs[i]->getId(), m_history);
                if (hasError && std::rand() < errorThreshold) aI = std::rand() % 2;
                if (hasError && std::rand() < errorThreshold) aJ = std::rand() % 2;
                m_history.recordInteraction(m_npcs[i]->getId(), m_npcs[j]->getId(), aI, aJ);
                int sI, sJ; calculateScores(aI, aJ, sI, sJ);
                m_npcs[i]->addScore(sI); m_npcs[j]->addScore(sJ);
                m_npcs[i]->postRoundUpdate(m_npcs[j]->getId(), aJ, aI, sI);
                m_npcs[j]->postRoundUpdate(m_npcs[i]->getId(), aI, aJ, sJ);
            }
        m_currentRound++;
        if (m_eliminationInterval > 0 && m_currentRound % m_eliminationInterval == 0) {
            applyElimination(); adjustScores();
        }
        emit autoEvoStep(m_currentRound, m_autoEvoTargetRound, buildRankings());
        if (m_currentRound >= m_autoEvoTargetRound) {
            stopAutoEvolution(); emit autoEvoFinished(buildRankings());
        }
        return;
    }

    // ====== 逐对演化（一对/tick） ======
    // 等待折算步
    if (m_autoEvoWaitingAdjust) {
        adjustScores();
        m_autoEvoWaitingAdjust = false;
        m_autoEvoTimer->setInterval(100);
        if (m_autoEvoSingleRound) {
            m_autoEvoSingleRound = false;
            m_autoEvoTimer->stop();
            m_autoEvoPaused = true;
        }
        emit autoEvoStep(m_currentRound, m_autoEvoTargetRound, buildRankings());
        return;
    }

    // 正在调整步 → 推进到下一回合
    if (m_autoEvoAdjustingRound) {
        m_autoEvoAdjustingRound = false;
        m_currentRound++;
        if (m_currentRound >= m_autoEvoTargetRound) {
            stopAutoEvolution(); emit autoEvoFinished(buildRankings());
            return;
        }
        applyElimination();
        m_autoEvoWaitingAdjust = true;
        m_autoEvoTimer->setInterval(400);
        emit autoEvoStep(m_currentRound, m_autoEvoTargetRound, buildRankings());
        emit autoEvoPairDone(-1, -1);
        return;
    }

    int n = static_cast<int>(m_npcs.size());
    if (n < 2) return;

    // 找下一对
    while (m_autoEvoPairI < n && m_autoEvoPairJ >= n) {
        m_autoEvoPairI++; m_autoEvoPairJ = m_autoEvoPairI + 1;
    }
    if (m_autoEvoPairI >= n || m_autoEvoPairJ >= n) {
        // 所有对完成
        bool elimDue = (m_eliminationInterval > 0 && (m_currentRound + 1) % m_eliminationInterval == 0);
        if (elimDue) {
            m_autoEvoAdjustingRound = true;
            emit autoEvoStep(m_currentRound + 1, m_autoEvoTargetRound, buildRankings());
            emit autoEvoPairDone(-1, -1);
        } else {
            m_currentRound++;
            if (m_currentRound >= m_autoEvoTargetRound) {
                if (m_autoEvoSingleRound) m_autoEvoSingleRound = false;
                stopAutoEvolution();
                emit autoEvoFinished(buildRankings());
                return;
            }
            if (m_autoEvoSingleRound) {
                m_autoEvoSingleRound = false;
                m_autoEvoTimer->stop();
                m_autoEvoPaused = true;
            }
            m_autoEvoPairI = 0; m_autoEvoPairJ = 1;
            emit autoEvoStep(m_currentRound, m_autoEvoTargetRound, buildRankings());
        }
        return;
    }

    int i = m_autoEvoPairI, j = m_autoEvoPairJ;
    bool hasError = (m_errorRate > 0.0);
    double errorThreshold = m_errorRate * RAND_MAX;
    int aI = m_npcs[i]->action(m_npcs[j]->getId(), m_history);
    int aJ = m_npcs[j]->action(m_npcs[i]->getId(), m_history);
    if (hasError && std::rand() < errorThreshold) aI = std::rand() % 2;
    if (hasError && std::rand() < errorThreshold) aJ = std::rand() % 2;
    m_history.recordInteraction(m_npcs[i]->getId(), m_npcs[j]->getId(), aI, aJ);
    int sI, sJ; calculateScores(aI, aJ, sI, sJ);
    m_npcs[i]->addScore(sI); m_npcs[j]->addScore(sJ);
    m_npcs[i]->postRoundUpdate(m_npcs[j]->getId(), aJ, aI, sI);
    m_npcs[j]->postRoundUpdate(m_npcs[i]->getId(), aI, aJ, sJ);

    m_autoEvoPairJ++;
    if (m_autoEvoPairJ >= n) { m_autoEvoPairI++; m_autoEvoPairJ = m_autoEvoPairI + 1; }

    emit autoEvoStep(m_currentRound + 1, m_autoEvoTargetRound, buildRankings());
    emit autoEvoPairDone(m_npcs[i]->getId(), m_npcs[j]->getId());
}
