#ifndef PERIODICNPC_H
#define PERIODICNPC_H

#include "npcbase.h"

// 周期者：按固定周期循环：合作→背叛→背叛→合作→重复（CCDD 四步循环）
// 首轮合作
class PeriodicNPC : public NPCBase {
public:
    PeriodicNPC(int id, const QString& name) : NPCBase(id, name) {
        m_strategyType = "周期者";
    }

    int action(int opponentId, const HistoryRecord& history) override {
        QVector<int> myActions = history.getActionsAgainst(m_id, opponentId);
        // 当前是第几轮（0-based）
        int round = myActions.size();
        // 固定序列：合作(0), 背叛(1), 背叛(1), 合作(0) → CDDC
        static const int seq[4] = {0, 1, 1, 0};
        return seq[round % 4];
    }
};

#endif // PERIODICNPC_H
