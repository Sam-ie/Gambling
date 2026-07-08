#ifndef INTERACTIONRUNNER_H
#define INTERACTIONRUNNER_H

#include "npcbase.h"
#include "historyrecord.h"
#include "payoffmatrix.h"
#include <random>
#include <memory>

// 交互执行器：封装"双方出牌 + 失误判定 + 记录 + 计分 + RL更新"的五步流程
// 消除 GameEngine 中 runNPCRound / autoEvoTick(Fast) / stepAutoEvolutionPair 三处重复代码
class InteractionRunner {
public:
    struct Result {
        int actionA, actionB;
        int scoreA, scoreB;
    };

    explicit InteractionRunner(double errorRate = 0.0);

    void setErrorRate(double rate) { m_errorRate = rate; }
    double errorRate() const { return m_errorRate; }

    // 执行一对 NPC 之间的单次交互
    // 返回双方的行为和得分
    Result run(NPCBase& npcA, NPCBase& npcB, HistoryRecord& history,
               const PayoffMatrix& payoff);

    // 执行所有 NPC 之间的完整一轮交互（无玩家参与）
    void runAllPairs(std::vector<std::unique_ptr<NPCBase>>& npcs,
                     HistoryRecord& history, const PayoffMatrix& payoff);

    // 玩家 vs NPC 单次交互（NPC 可能失误，玩家不受影响）
    Result playerVsNpc(int playerAction, NPCBase& npc,
                       int playerId, HistoryRecord& history,
                       const PayoffMatrix& payoff);

    // 统一随机数引擎
    std::mt19937& rng() { return m_rng; }

private:
    double m_errorRate;
    std::mt19937 m_rng;

    // 对 NPC 动作施加失误率（玩家不受影响）
    int applyError(int intendedAction);
};

#endif // INTERACTIONRUNNER_H
