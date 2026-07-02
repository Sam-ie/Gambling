#ifndef REPEATERNPC_H
#define REPEATERNPC_H

#include "npcbase.h"

// 复读者（Tit-for-Tat）：复制对手上一轮行为，首轮默认合作
class RepeaterNPC : public NPCBase {
public:
    RepeaterNPC(int id, const QString& name) : NPCBase(id, name) {
        m_strategyType = "复读者";
    }

    int action(int opponentId, const HistoryRecord& history) override {
        QVector<int> opponentActions = history.getActionsAgainst(opponentId, m_id);
        if (opponentActions.isEmpty()) {
            return 0; // 首轮默认诚实
        }
        return opponentActions.last(); // 以牙还牙
    }
};

#endif // REPEATERNPC_H
