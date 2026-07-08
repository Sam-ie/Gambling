#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include <QObject>
#include <QVector>
#include <memory>
#include <vector>

#include "npcscoreinfo.h"
#include "npc/npcbase.h"
#include "npc/npcfactory.h"
#include "npc/npcconfig.h"
#include "historyrecord.h"
#include "payoffmatrix.h"
#include "interactionrunner.h"
#include "elimination/eliminationmanager.h"
#include "evolution/autoevolutionengine.h"

class GameEngine : public QObject
{
    Q_OBJECT

public:
    enum Phase {
        IDLE,
        NPC_INTERACTION,
        WAITING_FOR_PLAYER,
        FINISHED
    };

    explicit GameEngine(QObject* parent = nullptr);

    // ---- 状态查询 ----
    Phase phase() const { return m_phase; }
    int currentRound() const { return m_currentRound; }
    int totalRounds() const { return m_totalRounds; }
    int playerScore() const { return m_playerScore; }
    int currentOpponentIndex() const { return m_currentOpponentIdx; }
    const std::vector<std::unique_ptr<NPCBase>>& npcs() const { return m_npcs; }

    // ---- 游戏控制 ----
    void initializeNPCs(const NPCConfig& config);
    void startGame(int totalRounds);
    void resetGame();

    void playerAction(int action);  // 0=合作, 1=背叛

    // ---- 配置 ----
    void setTotalRounds(int rounds) { m_totalRounds = rounds; }
    void setEliminationInterval(int rounds) { m_eliminator.setInterval(rounds); }
    int eliminationInterval() const { return m_eliminator.interval(); }
    void setEliminationCount(int cnt) { m_eliminator.setCount(cnt); }
    int eliminationCount() const { return m_eliminator.count(); }
    void setInheritHistory(bool enabled) { m_eliminator.setInheritHistory(enabled); }
    bool inheritHistory() const { return m_eliminator.inheritHistory(); }
    void setErrorRate(double rate);
    void setScoreInheritRatio(int pct);
    int scoreInheritRatio() const;

    // ---- 积分规则 ----
    void setScoreRules(int R, int T, int S, int P) { m_payoff.set(R, T, S, P); }
    int cooperateReward()  const { return m_payoff.cooperateReward(); }
    int cheatReward()      const { return m_payoff.cheatReward(); }
    int betrayedPenalty()  const { return m_payoff.betrayedPenalty(); }
    int bothCheatPenalty() const { return m_payoff.bothCheatPenalty(); }

    void setAutoEvolutionSpeed(int ms);

    // ---- 查询 ----
    QVector<InteractionPair> getPlayerInteractionHistory(int npcId) const;
    QVector<NPCScoreInfo> buildRankings() const;

    // ---- 作弊 ----
    void cheatAddScore(int points);
    void setNPCScores(const QVector<QPair<int, int>>& updates);

    // ---- 自动演化委托 ----
    void startAutoEvolution(int totalRounds, bool startTimer = true);
    void stopAutoEvolution()    { m_evoEngine.stop(); }
    void pauseAutoEvolution()   { m_evoEngine.pause(); }
    void resumeAutoEvolution()  { m_evoEngine.resume(); }
    void stepAutoEvolutionPair(){ if (!m_evoEngine.isRunning()) startAutoEvolution(m_totalRounds, false); m_evoEngine.stepPair(); }
    void stepAutoEvolution()    { if (!m_evoEngine.isRunning()) startAutoEvolution(m_totalRounds, false); m_evoEngine.stepRound(); }
    void startAutoEvolutionFast();
    bool autoEvoActive() const { return m_evoEngine.isRunning(); }
    bool autoEvoPaused() const { return m_evoEngine.isPaused(); }

    // ---- 暴露子模块（供演化引擎回调） ----
    InteractionRunner& runner()          { return m_runner; }
    PayoffMatrix&      payoff()          { return m_payoff; }
    EliminationManager& eliminator()     { return m_eliminator; }
    AutoEvolutionEngine& evoEngine()      { return m_evoEngine; }

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

    // 自动演化信号（从 AutoEvolutionEngine 转发）
    void autoEvoStep(int round, int totalRounds,
                     const QVector<NPCScoreInfo>& rankings);
    void autoEvoPairDone(int npcIdA, int npcIdB);
    void autoEvoFinished(const QVector<NPCScoreInfo>& finalRankings);

private:
    // ---- 游戏状态 ----
    std::vector<std::unique_ptr<NPCBase>> m_npcs;
    std::vector<NPCFactory::NPCType> m_npcTypes;
    HistoryRecord m_history;
    PayoffMatrix m_payoff;
    InteractionRunner m_runner;
    EliminationManager m_eliminator;
    AutoEvolutionEngine m_evoEngine;

    int m_playerScore = 0;
    int m_currentRound = 0;
    int m_totalRounds = 10;
    int m_nextNpcId = 0;
    Phase m_phase = IDLE;
    int m_currentOpponentIdx = 0;

    // ---- 内部方法 ----
    void runNPCRound();
    void advanceTurn();
    void applyEliminationIfDue();
};

#endif // GAMEENGINE_H
