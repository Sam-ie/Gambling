#ifndef DECEPTIVENPC_H
#define DECEPTIVENPC_H

#include "npcbase.h"

// 背叛者：始终选择欺骗
class DeceptiveNPC : public NPCBase {
public:
    DeceptiveNPC(int id, const QString& name) : NPCBase(id, name) {
        m_strategyType = "背叛者";
    }

    int action(int opponentId, const HistoryRecord& history) override {
        Q_UNUSED(opponentId);
        Q_UNUSED(history);
        return 1;
    }
};

#endif // DECEPTIVENPC_H
