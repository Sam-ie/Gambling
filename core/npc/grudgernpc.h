#ifndef GRUDGERNPC_H
#define GRUDGERNPC_H

#include "npcbase.h"
#include <QSet>

// 记仇者：首次合作；一旦被某对手背叛过，则对该对手永不原谅，始终背叛
class GrudgerNPC : public NPCBase {
public:
    GrudgerNPC(int id, const QString& name) : NPCBase(id, name) {
        m_strategyType = "记仇者";
    }

    int action(int opponentId, const HistoryRecord& history) override {
        if (m_betrayedBy.contains(opponentId)) {
            return 1; // 已被此对手背叛过，永不合作
        }
        // 检查历史上此对手是否背叛过自己
        QVector<int> oppActions = history.getActionsAgainst(opponentId, m_id);
        for (int act : oppActions) {
            if (act == 1) {
                m_betrayedBy.insert(opponentId);
                return 1;
            }
        }
        return 0; // 未被背叛，继续合作
    }

private:
    QSet<int> m_betrayedBy; // 记录曾背叛过自己的对手 ID
};

#endif // GRUDGERNPC_H
