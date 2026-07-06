#ifndef MAJORITYNPC_H
#define MAJORITYNPC_H

#include "npcbase.h"

// 从众者：统计对手历史中合作/背叛的次数，选择多数行为
// 平局时选择合作
class MajorityNPC : public NPCBase {
public:
    MajorityNPC(int id, const QString& name) : NPCBase(id, name) {
        m_strategyType = "从众者";
    }

    int action(int opponentId, const HistoryRecord& history) override {
        QVector<int> oppActions = history.getActionsAgainst(opponentId, m_id);
        if (oppActions.isEmpty()) {
            return 0; // 首轮合作
        }

        int coopCount = 0;
        int betrayCount = 0;
        for (int act : oppActions) {
            if (act == 0) coopCount++;
            else          betrayCount++;
        }

        // 平局合作
        return betrayCount > coopCount ? 1 : 0;
    }
};

#endif // MAJORITYNPC_H
