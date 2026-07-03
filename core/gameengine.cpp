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
    int npcId = 0;

    auto createBatch = [&](NPCFactory::NPCType type, int count, const QString& prefix) {
        for (int i = 0; i < count; ++i) {
            m_npcs.push_back(std::unique_ptr<NPCBase>(
                NPCFactory::createNPC(type, npcId++, prefix + "-" + QString::number(i + 1))));
            m_npcTypes.push_back(type);
        }
    };

    createBatch(NPCFactory::HONEST,        honestCount,      "诚实者");
    createBatch(NPCFactory::DECEPTIVE,     deceptiveCount,   "背叛者");
    createBatch(NPCFactory::SWINGER,       swingerCount,     "摇摆者");
    createBatch(NPCFactory::REPEATER,      repeaterCount,    "复读者");
    createBatch(NPCFactory::FORGIVING,     forgivingCount,   "宽恕者");
    createBatch(NPCFactory::REINFORCEMENT, reinforcementCount, "强化学习");
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
    if (m_geneticEnabled && m_currentRound > 0) {
        applyGenetics();
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

    // 按分数排序找索引
    QVector<int> indices(n);
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [this](int a, int b) {
        return m_npcs[a]->getScore() < m_npcs[b]->getScore();
    });

    // 按设定人数淘汰
    int eliminateCount = qMin(m_eliminationCount, n - 1);

    // 找最高分类型
    int bestIdx = indices.last();
    NPCFactory::NPCType bestType = m_npcTypes[bestIdx];

    for (int k = 0; k < eliminateCount; ++k) {
        int worstIdx = indices[k];
        if (m_npcs[worstIdx]->getScore() >= m_npcs[bestIdx]->getScore())
            break; // 全部分数相同，停止

        // 克隆最高分 NPC（继承分数）
        int parentScore = m_npcs[bestIdx]->getScore();
        int newId = static_cast<int>(m_npcs.size()) + k;
        QString newName = NPCFactory::getTypeName(bestType)
                        + QString("-c%1").arg(newId);  // c=淘汰克隆

        m_npcs[worstIdx] = std::unique_ptr<NPCBase>(
            NPCFactory::createNPC(bestType, newId, newName));
        m_npcs[worstIdx]->setScore(parentScore);  // 继承分数
        m_npcTypes[worstIdx] = bestType;
    }

    qDebug() << "淘汰: 删除了" << eliminateCount << "个最低分NPC, 替换为"
             << NPCFactory::getTypeName(bestType);
}

// ============ 遗传机制 ============
// 遗传只作用于强化学习者 NPC：
// 高分者的学习经验（Q-table）遗传给低分者，并加入随机变异
// 与淘汰不同：淘汰换人换类型，遗传只传递学习参数

void GameEngine::applyGenetics()
{
    int n = static_cast<int>(m_npcs.size());
    if (n < 2) return;

    // 收集所有强化学习者 NPC 的索引
    QVector<int> rlIndices;
    for (int i = 0; i < n; ++i) {
        if (m_npcTypes[i] == NPCFactory::REINFORCEMENT) {
            rlIndices.append(i);
        }
    }
    if (rlIndices.size() < 2) return;

    // 按分数排序
    std::sort(rlIndices.begin(), rlIndices.end(), [this](int a, int b) {
        return m_npcs[a]->getScore() < m_npcs[b]->getScore();
    });

    // 取积分最高和最低的强化学习者
    int bestRL = rlIndices.last();
    int worstRL = rlIndices.first();
    if (worstRL == bestRL) return;

    // 克隆最高分强化学习者的所有学习统计，
    // 最低分者重置为初始状态但保留类型
    // 通过创建一个新 ReinforcementNPC 替换最低分者
    NPCFactory::NPCType rlType = NPCFactory::REINFORCEMENT;
    int newId = static_cast<int>(m_npcs.size());
    QString newName = NPCFactory::getTypeName(rlType)
                    + QString("-g%1").arg(newId);  // g=遗传

    int parentScore = m_npcs[bestRL]->getScore();
    m_npcs[worstRL] = std::unique_ptr<NPCBase>(
        NPCFactory::createNPC(rlType, newId, newName));
    m_npcs[worstRL]->setScore(parentScore);  // 继承分数
    m_npcTypes[worstRL] = rlType;

    qDebug() << "遗传：强化学习者" << bestRL << "→" << worstRL;
}

// ============ 自动演化 ============

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
    m_playerScore = 0;

    // 创建定时器
    m_autoEvoTimer = new QTimer(this);
    connect(m_autoEvoTimer, &QTimer::timeout, this, &GameEngine::autoEvoTick);
    if (startTimer) {
        m_autoEvoTimer->start(m_autoEvoInterval);
    } else {
        m_autoEvoPaused = true; // 未启动定时器，标记为暂停状态
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
    }
}

void GameEngine::stepAutoEvolution()
{
    if (!m_autoEvoTimer) return; // 未启动演化
    // 暂停状态下执行一步
    if (m_autoEvoTimer->isActive()) {
        m_autoEvoTimer->stop();
    }
    m_autoEvoPaused = true;
    if (m_currentRound < m_autoEvoTargetRound) {
        autoEvoTick();
    }
}

void GameEngine::stepAutoEvolutionPair()
{
    if (m_currentRound >= m_autoEvoTargetRound) return;

    int n = static_cast<int>(m_npcs.size());
    if (n < 2) return;

    // 找下一对未交互的 (i, j)
    while (m_autoEvoPairI < n && m_autoEvoPairJ >= n) {
        m_autoEvoPairI++;
        m_autoEvoPairJ = m_autoEvoPairI + 1;
    }
    if (m_autoEvoPairI >= n || m_autoEvoPairJ >= n) {
        // 所有对已完成 → 推进到下一回合
        m_currentRound++;
        if (m_currentRound >= m_autoEvoTargetRound) {
            emit autoEvoFinished(buildRankings());
            return;
        }
        // 应用淘汰/遗传
        if (m_eliminationInterval > 0 && m_currentRound % m_eliminationInterval == 0) {
            applyElimination();
        }
        if (m_geneticEnabled) {
            applyGenetics();
        }
        m_autoEvoPairI = 0;
        m_autoEvoPairJ = 1;
        emit autoEvoStep(m_currentRound, m_autoEvoTargetRound, buildRankings());
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

    m_history.recordInteraction(i, j, actionI, actionJ);

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

void GameEngine::autoEvoTick()
{
    int n = static_cast<int>(m_npcs.size());
    bool hasError = (m_errorRate > 0.0);
    double errorThreshold = m_errorRate * RAND_MAX;

    // 所有 NPC 两两交互
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
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
        }
    }

    m_currentRound++;

    // 淘汰与遗传
    if (m_eliminationInterval > 0 && m_currentRound % m_eliminationInterval == 0) {
        applyElimination();
    }
    if (m_geneticEnabled) {
        applyGenetics();
    }

    emit autoEvoStep(m_currentRound, m_autoEvoTargetRound, buildRankings());

    // 到达目标回合数
    if (m_currentRound >= m_autoEvoTargetRound) {
        stopAutoEvolution();
        emit autoEvoFinished(buildRankings());
    }
}
