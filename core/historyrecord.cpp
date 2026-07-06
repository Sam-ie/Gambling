#include "historyrecord.h"

HistoryRecord::HistoryRecord() = default;

void HistoryRecord::ensureSize(int maxIdPlusOne)
{
    if (maxIdPlusOne <= m_capacity) return;
    int newSize = std::max(maxIdPlusOne, m_capacity * 2);
    m_history.resize(newSize);
    for (int i = 0; i < newSize; ++i) {
        if (i >= m_capacity) {
            m_history[i].resize(newSize);
        } else {
            m_history[i].resize(newSize);
        }
    }
    m_capacity = newSize;
}

void HistoryRecord::recordInteraction(int npcIdA, int npcIdB, int actionA, int actionB)
{
    int maxId = std::max(npcIdA, npcIdB) + 1;
    ensureSize(maxId);
    m_history[npcIdA][npcIdB].append({actionA, actionB});
    m_history[npcIdB][npcIdA].append({actionB, actionA});
}

QVector<InteractionPair> HistoryRecord::getInteractionHistory(int npcIdA, int npcIdB) const
{
    if (npcIdA >= 0 && npcIdA < m_capacity && npcIdB >= 0 && npcIdB < m_capacity) {
        return m_history[npcIdA][npcIdB];
    }
    return {};
}

QVector<int> HistoryRecord::getActionsAgainst(int npcIdA, int npcIdB) const
{
    QVector<int> actions;
    const auto history = getInteractionHistory(npcIdA, npcIdB);
    for (const auto& pair : history) {
        actions.append(pair.first);
    }
    return actions;
}

void HistoryRecord::clear()
{
    for (auto& row : m_history) {
        for (auto& cell : row) {
            cell.clear();
        }
    }
}

void HistoryRecord::copyHistory(int oldId, int newId)
{
    int maxId = std::max(oldId, newId) + 1;
    ensureSize(maxId);

    // 复制老 NPC 与其他各 NPC 的交互历史到新 NPC
    for (int k = 0; k < m_capacity; ++k) {
        if (k == oldId || k == newId) continue;
        m_history[newId][k] = m_history[oldId][k];     // 新 NPC 的视角
        m_history[k][newId] = m_history[k][oldId];     // 对方 NPC 的视角
    }
}

int HistoryRecord::capacity() const
{
    return m_capacity;
}
