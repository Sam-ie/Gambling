#include "presetmanager.h"
#include "spingroupcontroller.h"

PresetManager::PresetManager(SpinGroupController* controller, QObject* parent)
    : QObject(parent)
    , m_controller(controller)
{
    initPresets();
}

void PresetManager::initPresets()
{
    // 简单模式：仅4种基础类型，各2个
    m_easyConfig.set(NPCFactory::HONEST,    2);
    m_easyConfig.set(NPCFactory::DECEPTIVE, 2);
    m_easyConfig.set(NPCFactory::SWINGER,   2);
    m_easyConfig.set(NPCFactory::REPEATER,  2);

    // 普通模式：全部11种策略各1个
    for (const auto& type : NPCFactory::allTypes()) {
        m_normalConfig.set(type, 1);
    }

    // 困难模式：攻击型策略更多
    m_hardConfig.set(NPCFactory::HONEST,    1);
    m_hardConfig.set(NPCFactory::DECEPTIVE, 2);
    m_hardConfig.set(NPCFactory::SWINGER,   1);
    m_hardConfig.set(NPCFactory::REPEATER,  1);
    m_hardConfig.set(NPCFactory::FORGIVING, 1);
    m_hardConfig.set(NPCFactory::REINFORCEMENT, 1);
    m_hardConfig.set(NPCFactory::GRUDGER,   2);
    m_hardConfig.set(NPCFactory::DETECTIVE, 1);
    m_hardConfig.set(NPCFactory::PAVLOV,    1);
    m_hardConfig.set(NPCFactory::MAJORITY,  1);
    m_hardConfig.set(NPCFactory::PERIODIC,  1);

    // 经典模式：Axelrod 经典策略组合
    m_classicConfig.set(NPCFactory::HONEST,    2);
    m_classicConfig.set(NPCFactory::DECEPTIVE, 1);
    m_classicConfig.set(NPCFactory::SWINGER,   1);
    m_classicConfig.set(NPCFactory::REPEATER,  2);
    m_classicConfig.set(NPCFactory::FORGIVING, 1);
}

void PresetManager::apply(Preset preset)
{
    m_controller->applyConfig(configFor(preset));
}

const NPCConfig& PresetManager::configFor(Preset preset) const
{
    switch (preset) {
    case EASY:    return m_easyConfig;
    case NORMAL:  return m_normalConfig;
    case HARD:    return m_hardConfig;
    case CLASSIC: return m_classicConfig;
    }
    return m_normalConfig;
}
