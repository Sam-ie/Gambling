#ifndef ELIMINATIONMANAGER_H
#define ELIMINATIONMANAGER_H

#include "npcbase.h"
#include "npcfactory.h"
#include "historyrecord.h"
#include <memory>
#include <vector>
#include <random>

// 淘汰管理器：负责淘汰最低分 NPC + 克隆最高分 NPC + 积分折算
class EliminationManager {
public:
    explicit EliminationManager();

    void setInterval(int rounds) { m_interval = rounds; }
    int interval() const { return m_interval; }

    void setCount(int cnt) { m_count = cnt; }
    int count() const { return m_count; }

    void setInheritHistory(bool enabled) { m_inheritHistory = enabled; }
    bool inheritHistory() const { return m_inheritHistory; }

    void setScoreInheritRatio(double ratio) { m_scoreInheritRatio = ratio; }
    double scoreInheritRatio() const { return m_scoreInheritRatio; }

    // 判断当前回合是否需要淘汰
    bool shouldEliminate(int currentRound) const {
        return m_interval > 0 && currentRound > 0 && currentRound % m_interval == 0;
    }

    // 执行淘汰：删除 count 个最低分 NPC，替换为最高分类型的克隆
    // 返回淘汰次数（可能少于 count，当所有 NPC 分数相同时）
    int apply(std::vector<std::unique_ptr<NPCBase>>& npcs,
              std::vector<NPCFactory::NPCType>& types,
              int& nextNpcId, HistoryRecord& history);

    // 按比例折算所有 NPC 积分
    void adjustScores(std::vector<std::unique_ptr<NPCBase>>& npcs,
                      std::vector<std::unique_ptr<NPCBase>>*);

    // 折算玩家积分
    int adjustPlayerScore(int playerScore) const {
        return static_cast<int>(static_cast<double>(playerScore) * m_scoreInheritRatio);
    }

    std::mt19937& rng() { return m_rng; }

private:
    int m_interval = 2;
    int m_count = 1;
    bool m_inheritHistory = true;
    double m_scoreInheritRatio = 0.0;
    std::mt19937 m_rng;
};

#endif // ELIMINATIONMANAGER_H
