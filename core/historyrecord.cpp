#include "historyrecord.h"

HistoryRecord::HistoryRecord() = default;

void HistoryRecord::initialize(int participantCount)
{
    m_participantCount = participantCount;
    m_history.resize(participantCount);
    for (int i = 0; i < participantCount; ++i) {
        m_history[i].resize(participantCount);
    }
}

void HistoryRecord::recordInteraction(int a, int b, int actionA, int actionB)
{
    if (a >= 0 && a < m_participantCount && b >= 0 && b < m_participantCount) {
        m_history[a][b].append({actionA, actionB});
        m_history[b][a].append({actionB, actionA}); // 对称写入
    }
}

QVector<InteractionPair> HistoryRecord::getInteractionHistory(int a, int b) const
{
    if (a >= 0 && a < m_participantCount && b >= 0 && b < m_participantCount) {
        return m_history[a][b];
    }
    return {};
}

QVector<int> HistoryRecord::getActionsAgainst(int a, int b) const
{
    QVector<int> actions;
    const auto history = getInteractionHistory(a, b);
    for (const auto& pair : history) {
        actions.append(pair.first); // a 对 b 的行为
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

int HistoryRecord::participantCount() const
{
    return m_participantCount;
}
