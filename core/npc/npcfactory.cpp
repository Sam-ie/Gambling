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

// ---- 单一类型元数据表：消除 getTypeName / getAllTypes 重复 ----

struct TypeMeta {
    NPCFactory::NPCType type;
    const char* chineseName;
    const char* defaultPrefix;
};

static const TypeMeta kTypeMetas[] = {
    {NPCFactory::HONEST,        "诚实者",     "诚实者"},
    {NPCFactory::DECEPTIVE,     "背叛者",     "背叛者"},
    {NPCFactory::SWINGER,       "摇摆者",     "摇摆者"},
    {NPCFactory::REPEATER,      "复读者",     "复读者"},
    {NPCFactory::FORGIVING,     "宽恕者",     "宽恕者"},
    {NPCFactory::REINFORCEMENT, "强化学习者",  "强化学习"},
    {NPCFactory::GRUDGER,       "记仇者",     "记仇者"},
    {NPCFactory::DETECTIVE,     "试探者",     "试探者"},
    {NPCFactory::PAVLOV,        "趋利者",     "趋利者"},
    {NPCFactory::MAJORITY,      "从众者",     "从众者"},
    {NPCFactory::PERIODIC,      "周期者",     "周期者"},
};

// 编译期保证数量正确
static_assert(sizeof(kTypeMetas) / sizeof(kTypeMetas[0]) == NPCFactory::TYPE_COUNT,
              "TypeMeta count must match NPCType enum");

static const TypeMeta* metaFor(NPCFactory::NPCType type) {
    int idx = static_cast<int>(type);
    if (idx < 0 || idx >= NPCFactory::TYPE_COUNT) return nullptr;
    return &kTypeMetas[idx];
}

// ---- 工厂方法 ----

std::unique_ptr<NPCBase> NPCFactory::create(NPCType type, int id, const QString& name)
{
    switch (type) {
    case HONEST:        return std::make_unique<HonestNPC>(id, name);
    case DECEPTIVE:     return std::make_unique<DeceptiveNPC>(id, name);
    case SWINGER:       return std::make_unique<SwingerNPC>(id, name);
    case REPEATER:      return std::make_unique<RepeaterNPC>(id, name);
    case FORGIVING:     return std::make_unique<ForgivingNPC>(id, name);
    case REINFORCEMENT: return std::make_unique<ReinforcementNPC>(id, name);
    case GRUDGER:       return std::make_unique<GrudgerNPC>(id, name);
    case DETECTIVE:     return std::make_unique<DetectiveNPC>(id, name);
    case PAVLOV:        return std::make_unique<PavlovNPC>(id, name);
    case MAJORITY:      return std::make_unique<MajorityNPC>(id, name);
    case PERIODIC:      return std::make_unique<PeriodicNPC>(id, name);
    }
    return nullptr;
}

QString NPCFactory::typeName(NPCType type)
{
    auto* m = metaFor(type);
    return m ? QString::fromUtf8(m->chineseName) : QStringLiteral("未知");
}

QString NPCFactory::defaultPrefix(NPCType type)
{
    auto* m = metaFor(type);
    return m ? QString::fromUtf8(m->defaultPrefix) : QStringLiteral("未知");
}

const QVector<NPCFactory::NPCType>& NPCFactory::allTypes()
{
    static QVector<NPCType> types;
    if (types.isEmpty()) {
        types.reserve(TYPE_COUNT);
        for (const auto& m : kTypeMetas)
            types.append(m.type);
    }
    return types;
}
