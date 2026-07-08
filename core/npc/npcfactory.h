#ifndef NPCFACTORY_H
#define NPCFACTORY_H

#include "npcbase.h"
#include <memory>
#include <QMap>
#include <QString>

class NPCFactory
{
public:
    enum NPCType {
        HONEST,        // 诚实者
        DECEPTIVE,     // 背叛者
        SWINGER,       // 摇摆者
        REPEATER,      // 复读者
        FORGIVING,     // 宽恕者（Tit-for-Two-Tats）
        REINFORCEMENT, // 强化学习者
        GRUDGER,       // 记仇者
        DETECTIVE,     // 试探者
        PAVLOV,        // 趋利者（Win-Stay Lose-Shift）
        MAJORITY,      // 从众者
        PERIODIC        // 周期者
    };

    // 类型数量常量
    static constexpr int TYPE_COUNT = 11;

    // 工厂方法：返回 unique_ptr
    static std::unique_ptr<NPCBase> create(NPCType type, int id, const QString& name);

    // 获取类型的中文名称
    static QString typeName(NPCType type);

    // 获取类型对应的默认名称前缀
    static QString defaultPrefix(NPCType type);

    // 获取所有类型列表
    static const QVector<NPCType>& allTypes();
};

#endif // NPCFACTORY_H
