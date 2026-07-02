#ifndef FORGIVINGNPC_H
#define FORGIVINGNPC_H

#include "npcbase.h"

// 宽恕者：对手连续欺骗两次才以牙还牙（Tit-for-Two-Tats）
// 相比复读者更宽容，容忍一次背叛
class ForgivingNPC : public NPCBase {
public:
    ForgivingNPC(int id, const QString& name) : NPCBase(id, name) {
        m_strategyType = "宽恕者";
    }

    int action(int opponentId, const HistoryRecord& history) override {
        QVector<int> opponentActions = history.getActionsAgainst(opponentId, m_id);
        int n = opponentActions.size();

        if (n < 2) {
            return 0; // 前两轮默认合作
        }
        // 对手连续两轮欺骗才报复
        if (opponentActions[n - 1] == 1 && opponentActions[n - 2] == 1) {
            return 1;
        }
        return 0;
    }
};

#endif // FORGIVINGNPC_H
