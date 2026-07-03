#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include <QObject>
#include <QTimer>
#include <QVector>
#include <QPair>
#include <QString>
#include <memory>
#include <vector>

#include "npcbase.h"
#include "npcfactory.h"
#include "historyrecord.h"

// 单个 NPC 的概要信息（供 UI 展示）
struct NPCScoreInfo {
    int id;
    QString name;
    QString strategyType;
    int score;
};

class GameEngine : public QObject
{
    Q_OBJECT

public:
    enum Phase {
        IDLE,               // 未开始
        NPC_INTERACTION,    // NPC 间交互中
        WAITING_FOR_PLAYER, // 等待玩家选择
        ROUND_END,          // 回合结束
        FINISHED            // 游戏结束
    };

    explicit GameEngine(QObject* parent = nullptr);
    ~GameEngine() override;

    // ---- 状态查询 ----
    Phase phase() const { return m_phase; }
    int currentRound() const { return m_currentRound; }
    int totalRounds() const { return m_totalRounds; }
    int playerScore() const { return m_playerScore; }
    int currentOpponentIndex() const { return m_currentOpponentIdx; }
    const std::vector<std::unique_ptr<NPCBase>>& npcs() const { return m_npcs; }

    // ---- 游戏控制 ----
    // 根据 UI 中设置的各类型数量初始化 NPC
    void initializeNPCs(int honestCount, int deceptiveCount, int swingerCount,
                        int repeaterCount, int delayedCount, int reinforcementCount);
    void startGame(int totalRounds);
    void resetGame();

    // 玩家行动：0=诚实, 1=欺骗
    void playerAction(int action);

    // 运行 NPC 间的完整一轮交互
    void runNPCRound();

    // ---- 高级设置 ----
    void setTotalRounds(int rounds) { m_totalRounds = rounds; }
    void setEliminationInterval(int rounds) { m_eliminationInterval = rounds; }
    int eliminationInterval() const { return m_eliminationInterval; }
    void setInheritHistory(bool enabled) { m_inheritHistory = enabled; }
    bool inheritHistory() const { return m_inheritHistory; }
    void setAutoEvolutionSpeed(int ms) { m_autoEvoInterval = ms; }
    void setErrorRate(double rate) { m_errorRate = rate; }
    double errorRate() const { return m_errorRate; }
    void setScoreInheritRatio(int pct) { m_scoreInheritRatio = pct / 100.0; }
    int scoreInheritRatio() const { return static_cast<int>(m_scoreInheritRatio * 100); }

    // ---- 积分规则配置 ----
    void setScoreRules(int cooperateReward, int cheatReward,
                       int betrayedPenalty, int bothCheatPenalty) {
        m_cooperateReward = cooperateReward;
        m_cheatReward = cheatReward;
        m_betrayedPenalty = betrayedPenalty;
        m_bothCheatPenalty = bothCheatPenalty;
    }
    int cooperateReward() const { return m_cooperateReward; }
    int cheatReward() const { return m_cheatReward; }
    int betrayedPenalty() const { return m_betrayedPenalty; }
    int bothCheatPenalty() const { return m_bothCheatPenalty; }

    // ---- 淘汰人数 ----
    void setEliminationCount(int cnt) { m_eliminationCount = qBound(1, cnt, 25); }
    int eliminationCount() const { return m_eliminationCount; }

    // 查询玩家与某NPC的交互历史
    QVector<InteractionPair> getPlayerInteractionHistory(int npcId) const;

    void cheatAddScore(int points);

    // 作弊：设定指定 NPC 的分数
    void setNPCScores(const QVector<QPair<int, int>>& npcScoreUpdates);

    // 自动演化控制
    void pauseAutoEvolution();
    void resumeAutoEvolution();
    void stepAutoEvolution();     // 单轮（逐对回显，定时器驱动）
    void stepAutoEvolutionPair(); // 步进（逐对回显，手动点击）
    void startAutoEvolutionFast(); // 极速演化（一轮一帧）

    // 自动演化：纯 NPC 模拟，无玩家参与
    // 通过 QTimer 分步运行，每步发出 autoEvoStep 信号
    void startAutoEvolution(int totalRounds, int honestCount, int deceptiveCount,
                            int swingerCount, int repeaterCount,
                            int forgivingCount, int reinforcementCount,
                            bool startTimer = true);
    void stopAutoEvolution();
    bool autoEvoActive() const { return m_autoEvoTimer != nullptr; }

    // 单步淘汰（删除最低分，克隆最高分）
    void applyElimination();


signals:
    void gameStarted();
    void gameFinished(int finalPlayerScore);
    void roundChanged(int currentRound, int totalRounds);
    void playerTurn(int opponentId, const QString& opponentName,
                    const QString& opponentStrategy);
    void turnResult(int opponentId, const QString& opponentName,
                    int yourAction, int opponentAction,
                    int yourScoreChange, int opponentScoreChange);
    void allScoresUpdated(const QVector<NPCScoreInfo>& rankings);
    void npcInteractionsComplete();

    // 自动演化信号
    void autoEvoStep(int round, int totalRounds,
                     const QVector<NPCScoreInfo>& rankings);
    void autoEvoPairDone(int npcIdA, int npcIdB); // 一对交互完成
    void autoEvoFinished(const QVector<NPCScoreInfo>& finalRankings);

public:
    // 构建积分排名（UI 需要）
    QVector<NPCScoreInfo> buildRankings() const;

private:
    std::vector<std::unique_ptr<NPCBase>> m_npcs;
    std::vector<NPCFactory::NPCType> m_npcTypes;  // 并行记录每个 NPC 的类型
    HistoryRecord m_history;

    int m_playerScore = 0;
    int m_currentRound = 0;
    int m_totalRounds = 10;
    int m_nextNpcId = 0;  // 全局 NPC ID 计数器，避免与玩家位冲突
    Phase m_phase = IDLE;
    int m_currentOpponentIdx = 0;

    // 高级设置
    int m_eliminationInterval = 2;  // 每 N 回合
    int m_eliminationCount = 1;      // 每次淘汰人数
    bool m_inheritHistory = true;  // 克隆是否继承历史
    double m_scoreInheritRatio = 0.0; // 每轮后积分继承比例 0.0~1.0
    double m_errorRate = 0.0;       // 失误率 0.0~1.0
    int m_autoEvoInterval = 100;    // 自动演化每步间隔 ms

    // 可配置积分规则（标准囚徒困境：R=3, T=5, S=0, P=1）
    int m_cooperateReward = 3;      // 双方合作各得
    int m_cheatReward = 5;          // 欺骗方得
    int m_betrayedPenalty = 0;      // 被背叛方得
    int m_bothCheatPenalty = 1;     // 双方欺骗各得

    // 自动演化状态
    QTimer* m_autoEvoTimer = nullptr;
    int m_autoEvoTargetRound = 0;
    bool m_autoEvoPaused = false;
    int m_autoEvoPairI = 0;  // 步进模式：当前交互对索引
    int m_autoEvoPairJ = 1;
    bool m_autoEvoAdjustingRound = false;
    bool m_autoEvoWaitingAdjust = false; // 等待折算步（延时 0.3s）
    bool m_autoEvoFastMode = false;     // true=极速演化(一轮一帧) false=逐对演化
    bool m_autoEvoSingleRound = false;  // 单轮模式，一轮后停止

    // 支付矩阵（非静态，使用成员变量）
    void calculateScores(int action1, int action2,
                         int& score1, int& score2);

    // 推进到下一个对手或下一个回合
    void advanceTurn();

    // 自动演化单步
    void autoEvoTick();
    // 按比例折算积分
    void adjustScores();
};

#endif // GAMEENGINE_H
