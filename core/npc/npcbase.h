#ifndef NPCBASE_H
#define NPCBASE_H

#include "historyrecord.h"
#include <QString>

class NPCBase
{
public:
    NPCBase(int id, const QString& name);
    virtual ~NPCBase() = default;

    // 纯虚函数：根据历史记录决定行为
    // 返回0=诚实，1=欺骗
    virtual int action(int opponentId, const HistoryRecord& history) = 0;

    // 每轮交互后的学习回调（默认空实现，ReinforcementNPC 重写）
    virtual void postRoundUpdate(int opponentId, int opponentAction,
                                 int myAction, int reward) {
        Q_UNUSED(opponentId);
        Q_UNUSED(opponentAction);
        Q_UNUSED(myAction);
        Q_UNUSED(reward);
    }

    // 获取NPC信息
    int getId() const { return m_id; }
    QString getName() const { return m_name; }
    QString getStrategyType() const { return m_strategyType; }

    // 积分管理
    void addScore(int points) { m_score += points; }
    int getScore() const { return m_score; }
    void setScore(int score) { m_score = score; }
    void resetScore() { m_score = 0; }

protected:
    int m_id;
    QString m_name;
    QString m_strategyType;
    int m_score;
};

#endif // NPCBASE_H
