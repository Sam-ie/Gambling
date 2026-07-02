#ifndef HISTORYRECORD_H
#define HISTORYRECORD_H

#include <QVector>
#include <QPair>

// ---- 类型别名：降低嵌套模板可读性成本 ----
using InteractionPair = QPair<int, int>;           // (行为1, 行为2)
using InteractionHistory = QVector<InteractionPair>; // 一对玩家间的历史

class HistoryRecord
{
public:
    HistoryRecord();

    // 初始化历史记录（参与者总数，含玩家槽位）
    void initialize(int participantCount);

    // 记录一次交互行为
    void recordInteraction(int a, int b, int actionA, int actionB);

    // 获取两个参与者之间的历史交互记录
    QVector<InteractionPair> getInteractionHistory(int a, int b) const;

    // 获取参与者 a 对参与者 b 的历史行为序列
    QVector<int> getActionsAgainst(int a, int b) const;

    // 清空所有历史记录
    void clear();

    // 获取参与者数量
    int participantCount() const;

private:
    // m_history[a][b] = a 与 b 的交互历史（按轮次排列）
    QVector<QVector<InteractionHistory>> m_history;
    int m_participantCount = 0;
};

#endif // HISTORYRECORD_H
