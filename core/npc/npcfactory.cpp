#include "npcfactory.h"
#include "honestnpc.h"
#include "deceptivenpc.h"
#include "swingernpc.h"
#include "repeaternpc.h"
#include "forgivingnpc.h"
#include "reinforcementnpc.h"

NPCBase* NPCFactory::createNPC(NPCType type, int id, const QString& name)
{
    switch (type) {
    case HONEST:        return new HonestNPC(id, name);
    case DECEPTIVE:     return new DeceptiveNPC(id, name);
    case SWINGER:       return new SwingerNPC(id, name);
    case REPEATER:      return new RepeaterNPC(id, name);
    case FORGIVING:     return new ForgivingNPC(id, name);
    case REINFORCEMENT: return new ReinforcementNPC(id, name);
    default:            return nullptr;
    }
}

QString NPCFactory::getTypeName(NPCType type)
{
    static QMap<NPCType, QString> typeNames = {
        {HONEST,        "诚实者"},
        {DECEPTIVE,     "背叛者"},
        {SWINGER,       "摇摆者"},
        {REPEATER,      "复读者"},
        {FORGIVING,     "宽恕者"},
        {REINFORCEMENT, "强化学习者"}
    };
    return typeNames.value(type, "未知");
}

QMap<NPCFactory::NPCType, QString> NPCFactory::getAllTypes()
{
    return {
        {HONEST,        "诚实者"},
        {DECEPTIVE,     "背叛者"},
        {SWINGER,       "摇摆者"},
        {REPEATER,      "复读者"},
        {FORGIVING,     "宽恕者"},
        {REINFORCEMENT, "强化学习者"}
    };
}
