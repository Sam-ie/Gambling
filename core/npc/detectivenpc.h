#ifndef DETECTIVENPC_H
#define DETECTIVENPC_H

#include "npcbase.h"
#include <QSet>

// 试探者：前4轮按 合作→背叛→合作→合作 试探对手
// 试探期内若对手有背叛行为 → 转入以牙还牙（TFT）
// 试探期后未见背叛 → 一直背叛（对手太善良，全力剥削）
class DetectiveNPC : public NPCBase {
public:
    DetectiveNPC(int id, const QString& name) : NPCBase(id, name) {
        m_strategyType = "试探者";
    }

    int action(int opponentId, const HistoryRecord& history) override {
        QVector<int> oppActions = history.getActionsAgainst(opponentId, m_id);
        int round = oppActions.size(); // 0-based 回合索引

        // 前 4 轮：按固定序列
        if (round < 4) {
            static const int probeSeq[4] = {0, 1, 0, 0}; // 合作,背叛,合作,合作
            return probeSeq[round];
        }

        // 试探结果判定：检查前 4 轮中对手是否有背叛
        if (!m_probeDone.contains(opponentId)) {
            m_probeDone.insert(opponentId);
            bool opponentEverBetrayed = false;
            int checkRounds = qMin(4, oppActions.size());
            for (int i = 0; i < checkRounds; ++i) {
                if (oppActions[i] == 1) {
                    opponentEverBetrayed = true;
                    break;
                }
            }
            m_useTFT[opponentId] = opponentEverBetrayed;
        }

        // 根据试探结果选择策略
        if (m_useTFT.value(opponentId, false)) {
            // 以牙还牙
            return oppActions.last();
        } else {
            // 一直背叛
            return 1;
        }
    }

private:
    QSet<int> m_probeDone;
    QMap<int, bool> m_useTFT; // true=使用TFT, false=一直背叛
};

#endif // DETECTIVENPC_H
