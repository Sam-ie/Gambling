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
                NPCFactory::createNPC(type, npcId++, prefix + QString::number(i + 1))));
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
    m_history.initialize(playerSlot + 1);

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

            m_history.recordInteraction(i, j, actionI, actionJ);

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

    // 计算要淘汰多少个（按百分比）
    int eliminateCount = qMax(1, n * m_eliminationPercent / 100);

    // 找最高分类型
    int bestIdx = indices.last();
    NPCFactory::NPCType bestType = m_npcTypes[bestIdx];

    for (int k = 0; k < eliminateCount && k < n - 1; ++k) {
        int worstIdx = indices[k];
        if (m_npcs[worstIdx]->getScore() >= m_npcs[bestIdx]->getScore())
            break; // 全部分数相同，停止

        int newId = static_cast<int>(m_npcs.size()) + k;
        QString newName = NPCFactory::getTypeName(bestType)
                        + QString("(克隆%1)").arg(newId);

        m_npcs[worstIdx] = std::unique_ptr<NPCBase>(
            NPCFactory::createNPC(bestType, newId, newName));
        m_npcTypes[worstIdx] = bestType;
    }

    qDebug() << "淘汰: 删除了" << eliminateCount << "个最低分NPC, 替换为"
             << NPCFactory::getTypeName(bestType);
}

// ============ 遗传机制 ============

void GameEngine::applyGenetics()
{
    int n = static_cast<int>(m_npcs.size());
    if (n < 2) return;

    // 计算总分
    int totalScore = 0;
    int minScore = std::numeric_limits<int>::max();
    for (const auto& npc : m_npcs) {
        int s = npc->getScore();
        totalScore += s;
        if (s < minScore) minScore = s;
    }

    // 确保非负：将所有分数偏移到正数范围
    int offset = (minScore < 0) ? (-minScore + 1) : 0;
    int adjustedTotal = totalScore + offset * n;
    if (adjustedTotal <= 0) return;

    // 按分数比例随机选择父代
    static std::mt19937 rng(std::time(nullptr));
    std::uniform_int_distribution<int> dist(0, adjustedTotal - 1);
    int roll = dist(rng);

    int cumulative = 0;
    int parentIdx = 0;
    for (int i = 0; i < n; ++i) {
        cumulative += m_npcs[i]->getScore() + offset;
        if (roll < cumulative) {
            parentIdx = i;
            break;
        }
    }

    // 找最低分个体替换
    int minIdx = 0;
    for (int i = 1; i < n; ++i) {
        if (m_npcs[i]->getScore() < m_npcs[minIdx]->getScore()) minIdx = i;
    }
    if (minIdx == parentIdx) return;

    // 复制父代，替换最低分
    NPCFactory::NPCType parentType = m_npcTypes[parentIdx];
    int newId = static_cast<int>(m_npcs.size());
    QString newName = NPCFactory::getTypeName(parentType) + QString("(遗传%1)").arg(newId);

    m_npcs[minIdx] = std::unique_ptr<NPCBase>(
        NPCFactory::createNPC(parentType, newId, newName));
    m_npcTypes[minIdx] = parentType;
}

// ============ 自动演化 ============

void GameEngine::startAutoEvolution(int totalRounds, int honestCount,
                                     int deceptiveCount, int swingerCount,
                                     int repeaterCount, int forgivingCount,
                                     int reinforcementCount)
{
    stopAutoEvolution();

    // 初始化 NPC（无玩家参与）
    initializeNPCs(honestCount, deceptiveCount, swingerCount,
                    repeaterCount, forgivingCount, reinforcementCount);

    for (auto& npc : m_npcs) {
        npc->resetScore();
    }

    int participantCount = static_cast<int>(m_npcs.size());
    m_history.initialize(participantCount);

    m_currentRound = 0;
    m_autoEvoTargetRound = totalRounds;
    m_playerScore = 0;
    m_phase = IDLE;

    // 创建定时器，逐步运行
    m_autoEvoTimer = new QTimer(this);
    connect(m_autoEvoTimer, &QTimer::timeout, this, &GameEngine::autoEvoTick);
    m_autoEvoTimer->start(m_autoEvoInterval);

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
    // 暂停状态下执行一步（不启动 timer）
    bool wasRunning = m_autoEvoTimer->isActive() || !m_autoEvoPaused;
    if (m_autoEvoTimer->isActive()) {
        m_autoEvoTimer->stop();
    }
    m_autoEvoPaused = true;
    if (m_currentRound < m_autoEvoTargetRound) {
        autoEvoTick();
    }
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

            m_history.recordInteraction(i, j, actionI, actionJ);

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
