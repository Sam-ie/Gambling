#include "eliminationmanager.h"
#include <algorithm>
#include <numeric>
#include <limits>
#include <chrono>
#include <QSet>

EliminationManager::EliminationManager()
{
    m_rng.seed(static_cast<unsigned>(std::chrono::steady_clock::now()
                                     .time_since_epoch().count()));
}

int EliminationManager::apply(std::vector<std::unique_ptr<NPCBase>>& npcs,
                               std::vector<NPCFactory::NPCType>& types,
                               int& nextNpcId, HistoryRecord& history)
{
    int n = static_cast<int>(npcs.size());
    if (n < 2) return 0;

    int eliminateCount = qMin(m_count, n - 1);
    if (eliminateCount <= 0) return 0;

    // 找最高分者（随机打破平局）
    int maxScore = std::numeric_limits<int>::min();
    for (auto& npc : npcs) maxScore = qMax(maxScore, npc->getScore());

    QVector<int> bestPool;
    for (int i = 0; i < n; ++i) {
        if (npcs[i]->getScore() == maxScore) bestPool.append(i);
    }
    int bestIdx = bestPool[std::uniform_int_distribution<int>(0,
        static_cast<int>(bestPool.size()) - 1)(m_rng)];

    NPCFactory::NPCType bestType = types[bestIdx];
    int parentScore = npcs[bestIdx]->getScore();

    // 找最低分淘汰对象
    QVector<int> sortedByScore(n);
    std::iota(sortedByScore.begin(), sortedByScore.end(), 0);
    std::sort(sortedByScore.begin(), sortedByScore.end(),
              [&npcs](int a, int b) {
                  return npcs[a]->getScore() < npcs[b]->getScore();
              });

    QSet<int> eliminated;
    int actualElim = 0;
    for (int k = 0; k < eliminateCount; ++k) {
        int curMinScore = std::numeric_limits<int>::max();
        for (int i = 0; i < n; ++i) {
            if (eliminated.contains(i)) continue;
            curMinScore = qMin(curMinScore, npcs[i]->getScore());
        }
        QVector<int> pool;
        for (int i = 0; i < n; ++i) {
            if (eliminated.contains(i)) continue;
            if (npcs[i]->getScore() == curMinScore) pool.append(i);
        }
        int worstIdx = pool[std::uniform_int_distribution<int>(0,
            static_cast<int>(pool.size()) - 1)(m_rng)];
        eliminated.insert(worstIdx);

        if (curMinScore >= parentScore) break;

        int oldId = npcs[worstIdx]->getId();
        int newId = nextNpcId++;
        QString newName = NPCFactory::typeName(bestType)
                        + QString("-c%1").arg(newId);
        npcs[worstIdx] = NPCFactory::create(bestType, newId, newName);
        npcs[worstIdx]->setScore(parentScore);
        types[worstIdx] = bestType;
        actualElim++;

        if (m_inheritHistory) {
            history.copyHistory(oldId, newId);
        }
    }

    return actualElim;
}

void EliminationManager::adjustScores(
    std::vector<std::unique_ptr<NPCBase>>& npcs,
    std::vector<std::unique_ptr<NPCBase>>*)
{
    if (m_scoreInheritRatio >= 1.0) return;
    for (auto& npc : npcs) {
        npc->setScore(static_cast<int>(
            static_cast<double>(npc->getScore()) * m_scoreInheritRatio));
    }
}
