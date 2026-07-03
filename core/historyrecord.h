#ifndef HISTORYRECORD_H
#define HISTORYRECORD_H

#include <QVector>
#include <QPair>
#include <algorithm>

// ---- 类型别名：降低嵌套模板可读性成本 ----
using InteractionPair = QPair<int, int>;           // (行为1, 行为2)
using InteractionHistory = QVector<InteractionPair>; // 一对玩家间的历史

class HistoryRecord
{
public:
    HistoryRecord();

    // 确保矩阵能容纳最大 NPC ID（动态扩容，ID 递增时 O(1) 均摊）
    void ensureSize(int maxIdPlusOne);

    // 记录一次交互（a, b 为 NPC 的 ID，不是数组下标）
    void recordInteraction(int npcIdA, int npcIdB, int actionA, int actionB);

    // 获取两个参与者之间的历史交互记录
    QVector<InteractionPair> getInteractionHistory(int npcIdA, int npcIdB) const;

    // 获取参与者 a 对参与者 b 的历史行为序列
    QVector<int> getActionsAgainst(int npcIdA, int npcIdB) const;

    // 清空所有历史记录
    void clear();

    // 当前矩阵容量
    int capacity() const;

private:
    // m_history[a][b] = a 与 b 的交互历史（a, b 为 NPC ID）
    QVector<QVector<InteractionHistory>> m_history;
    int m_capacity = 0;
};

#endif // HISTORYRECORD_H
