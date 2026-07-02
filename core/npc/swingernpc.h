#ifndef SWINGERNPC_H
#define SWINGERNPC_H

#include "npcbase.h"
#include <cstdlib>

// 摇摆者：随机选择合作或欺骗
class SwingerNPC : public NPCBase {
public:
    SwingerNPC(int id, const QString& name) : NPCBase(id, name) {
        m_strategyType = "摇摆者";
    }

    int action(int opponentId, const HistoryRecord& history) override {
        Q_UNUSED(opponentId);
        Q_UNUSED(history);
        return std::rand() % 2;
    }
};

#endif // SWINGERNPC_H
