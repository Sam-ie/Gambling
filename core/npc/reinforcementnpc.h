#ifndef REINFORCEMENTNPC_H
#define REINFORCEMENTNPC_H

#include "npcbase.h"
#include <QMap>
#include <cstdlib>

// 强化学习者：基于历史收益的 ε-greedy 策略
// 为每个对手分别统计合作/欺骗的平均收益，选择平均收益更高的行为
class ReinforcementNPC : public NPCBase {
public:
    ReinforcementNPC(int id, const QString& name) : NPCBase(id, name) {
        m_strategyType = "强化学习者";
    }

    int action(int opponentId, const HistoryRecord& history) override {
        Q_UNUSED(history);
        if (!m_stats.contains(opponentId)) {
            // 首次遭遇：随机探索
            return std::rand() % 2;
        }

        const auto& stats = m_stats[opponentId];
        double honestAvg = stats.honestCount > 0
            ? static_cast<double>(stats.honestScore) / stats.honestCount : 0.0;
        double cheatAvg = stats.cheatCount > 0
            ? static_cast<double>(stats.cheatScore) / stats.cheatCount : 0.0;

        // ε-greedy：10% 概率随机探索
        if (static_cast<double>(std::rand()) / RAND_MAX < 0.1) {
            return std::rand() % 2;
        }

        return honestAvg >= cheatAvg ? 0 : 1;
    }

    // 每轮交互后更新统计数据
    void postRoundUpdate(int opponentId, int opponentAction, int myAction, int reward) override {
        Q_UNUSED(opponentAction);
        auto& stats = m_stats[opponentId];
        if (myAction == 0) {
            stats.honestCount++;
            stats.honestScore += reward;
        } else {
            stats.cheatCount++;
            stats.cheatScore += reward;
        }
    }

private:
    struct OpponentStats {
        int honestCount = 0;
        int honestScore = 0;
        int cheatCount = 0;
        int cheatScore = 0;
    };
    QMap<int, OpponentStats> m_stats;
};

#endif // REINFORCEMENTNPC_H
