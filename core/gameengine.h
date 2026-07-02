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
    void setGeneticEnabled(bool enabled) { m_geneticEnabled = enabled; }
    bool geneticEnabled() const { return m_geneticEnabled; }
    void setAutoEvolutionSpeed(int ms) { m_autoEvoInterval = ms; }
    void setErrorRate(double rate) { m_errorRate = rate; }
    double errorRate() const { return m_errorRate; }

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

    // ---- 淘汰比例 ----
    void setEliminationPercent(int pct) { m_eliminationPercent = qBound(1, pct, 100); }
    int eliminationPercent() const { return m_eliminationPercent; }

    void cheatAddScore(int points);

    // 作弊：设定指定 NPC 的分数
    void setNPCScores(const QVector<QPair<int, int>>& npcScoreUpdates);

    // 自动演化控制 - 暂停/继续/单步
    void pauseAutoEvolution();
    void resumeAutoEvolution();
    void stepAutoEvolution();  // 暂停状态下执行一步

    // 自动演化：纯 NPC 模拟，无玩家参与
    // 通过 QTimer 分步运行，每步发出 autoEvoStep 信号
    void startAutoEvolution(int totalRounds, int honestCount, int deceptiveCount,
                            int swingerCount, int repeaterCount,
                            int forgivingCount, int reinforcementCount);
    void stopAutoEvolution();

    // 单步淘汰（删除最低分，克隆最高分）
    void applyElimination();

    // 遗传：按积分比例繁殖高分 NPC
    void applyGenetics();

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
    void autoEvoFinished(const QVector<NPCScoreInfo>& finalRankings);

private:
    std::vector<std::unique_ptr<NPCBase>> m_npcs;
    std::vector<NPCFactory::NPCType> m_npcTypes;  // 并行记录每个 NPC 的类型
    HistoryRecord m_history;

    int m_playerScore = 0;
    int m_currentRound = 0;
    int m_totalRounds = 10;
    Phase m_phase = IDLE;
    int m_currentOpponentIdx = 0;

    // 高级设置
    int m_eliminationInterval = 0;  // 0=禁用
    int m_eliminationPercent = 100; // 淘汰比例，100=淘汰1个最低分
    bool m_geneticEnabled = false;
    double m_errorRate = 0.0;       // 失误率 0.0~1.0
    int m_autoEvoInterval = 200;    // 自动演化每步间隔 ms

    // 可配置积分规则（新默认：诚实各1分，欺骗2/-1分，双骗0分）
    int m_cooperateReward = 1;      // 双方合作各得
    int m_cheatReward = 2;          // 欺骗方得
    int m_betrayedPenalty = -1;     // 被背叛方得
    int m_bothCheatPenalty = 0;     // 双方欺骗各得

    // 自动演化状态
    QTimer* m_autoEvoTimer = nullptr;
    int m_autoEvoTargetRound = 0;
    bool m_autoEvoPaused = false;

    // 支付矩阵（非静态，使用成员变量）
    void calculateScores(int action1, int action2,
                         int& score1, int& score2);

    // 构建积分排名
    QVector<NPCScoreInfo> buildRankings() const;

    // 推进到下一个对手或下一个回合
    void advanceTurn();

    // 自动演化单步
    void autoEvoTick();
};

#endif // GAMEENGINE_H
