#ifndef HONESTNPC_H
#define HONESTNPC_H

#include "npcbase.h"

// 诚实者：始终选择合作
class HonestNPC : public NPCBase {
public:
    HonestNPC(int id, const QString& name) : NPCBase(id, name) {
        m_strategyType = "诚实者";
    }

    int action(int opponentId, const HistoryRecord& history) override {
        Q_UNUSED(opponentId);
        Q_UNUSED(history);
        return 0;
    }
};

#endif // HONESTNPC_H
