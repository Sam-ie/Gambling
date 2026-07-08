#ifndef NPCCONFIG_H
#define NPCCONFIG_H

#include "npcfactory.h"
#include <QVector>
#include <QString>

// NPC 配置条目：类型 + 数量 + 名称前缀
struct NPCConfigEntry {
    NPCFactory::NPCType type;
    int count = 0;

    NPCConfigEntry() : type(NPCFactory::HONEST) {}
    NPCConfigEntry(NPCFactory::NPCType t, int c) : type(t), count(c) {}
};

// NPC 配置：包含所有11种策略的数量设定
// 替代原来 GameEngine::initializeNPCs(11个参数)
class NPCConfig {
public:
    NPCConfig() { m_entries.fill(NPCConfigEntry(), 11); }

    void set(NPCFactory::NPCType type, int count) {
        int idx = static_cast<int>(type);
        if (idx >= 0 && idx < 11) m_entries[idx].count = count;
    }

    int get(NPCFactory::NPCType type) const {
        int idx = static_cast<int>(type);
        return (idx >= 0 && idx < 11) ? m_entries[idx].count : 0;
    }

    int totalCount() const {
        int sum = 0;
        for (const auto& e : m_entries) sum += e.count;
        return sum;
    }

    bool isEmpty() const { return totalCount() == 0; }

    // 遍历所有非零条目
    template<typename Func>
    void forEach(Func&& f) const {
        for (int i = 0; i < 11; ++i) {
            if (m_entries[i].count > 0)
                f(static_cast<NPCFactory::NPCType>(i), m_entries[i].count);
        }
    }

private:
    QVector<NPCConfigEntry> m_entries;
};

#endif // NPCCONFIG_H
