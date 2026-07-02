#ifndef NPCFACTORY_H
#define NPCFACTORY_H

#include "npcbase.h"
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
        REINFORCEMENT  // 强化学习者
    };

    static NPCBase* createNPC(NPCType type, int id, const QString& name);
    static QString getTypeName(NPCType type);
    static QMap<NPCType, QString> getAllTypes();
};

#endif // NPCFACTORY_H
