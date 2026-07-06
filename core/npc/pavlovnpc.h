#ifndef PAVLOVNPC_H
#define PAVLOVNPC_H

#include "npcbase.h"

// 趋利者（Win-Stay Lose-Shift）：
// 查看上一轮交互结果：若双方行为相同（都合作或都背叛）→ 合作
//                     若双方行为不同 → 背叛
// 首轮默认合作
class PavlovNPC : public NPCBase {
public:
    PavlovNPC(int id, const QString& name) : NPCBase(id, name) {
        m_strategyType = "趋利者";
    }

    int action(int opponentId, const HistoryRecord& history) override {
        QVector<InteractionPair> pairs = history.getInteractionHistory(m_id, opponentId);
        if (pairs.isEmpty()) {
            return 0; // 首轮合作
        }

        auto& last = pairs.last();
        int myLast = last.first;
        int oppLast = last.second;

        // 结果相同时维持（合作），不同时改变（背叛）
        if (myLast == oppLast) {
            return 0; // Win or Lose → Stay: 合作
        } else {
            return 1; // Lose or Win → Shift: 背叛
        }
    }
};

#endif // PAVLOVNPC_H
