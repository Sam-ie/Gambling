#include "npcfactory.h"
#include "honestnpc.h"
#include "deceptivenpc.h"
#include "swingernpc.h"
#include "repeaternpc.h"
#include "forgivingnpc.h"
#include "reinforcementnpc.h"
#include "grudgernpc.h"
#include "detectivenpc.h"
#include "pavlovnpc.h"
#include "majoritynpc.h"
#include "periodicnpc.h"

NPCBase* NPCFactory::createNPC(NPCType type, int id, const QString& name)
{
    switch (type) {
    case HONEST:        return new HonestNPC(id, name);
    case DECEPTIVE:     return new DeceptiveNPC(id, name);
    case SWINGER:       return new SwingerNPC(id, name);
    case REPEATER:      return new RepeaterNPC(id, name);
    case FORGIVING:     return new ForgivingNPC(id, name);
    case REINFORCEMENT: return new ReinforcementNPC(id, name);
    case GRUDGER:       return new GrudgerNPC(id, name);
    case DETECTIVE:     return new DetectiveNPC(id, name);
    case PAVLOV:        return new PavlovNPC(id, name);
    case MAJORITY:      return new MajorityNPC(id, name);
    case PERIODIC:      return new PeriodicNPC(id, name);
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
        {REINFORCEMENT, "强化学习者"},
        {GRUDGER,       "记仇者"},
        {DETECTIVE,     "试探者"},
        {PAVLOV,        "趋利者"},
        {MAJORITY,      "从众者"},
        {PERIODIC,      "周期者"}
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
        {REINFORCEMENT, "强化学习者"},
        {GRUDGER,       "记仇者"},
        {DETECTIVE,     "试探者"},
        {PAVLOV,        "趋利者"},
        {MAJORITY,      "从众者"},
        {PERIODIC,      "周期者"}
    };
}
