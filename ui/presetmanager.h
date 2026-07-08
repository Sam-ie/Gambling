#ifndef PRESETMANAGER_H
#define PRESETMANAGER_H

#include <QObject>
#include <QVector>
#include "../core/npc/npcconfig.h"

class SpinGroupController;

// 预设配置管理：维护4个预设方案（简单/普通/困难/经典）
// 从 Gambling 的 4 个 on_btnPreset* 函数中抽出
class PresetManager : public QObject
{
    Q_OBJECT

public:
    explicit PresetManager(SpinGroupController* controller, QObject* parent = nullptr);

    enum Preset { EASY, NORMAL, HARD, CLASSIC };

    void apply(Preset preset);
    const NPCConfig& configFor(Preset preset) const;

private:
    SpinGroupController* m_controller;
    NPCConfig m_easyConfig;
    NPCConfig m_normalConfig;
    NPCConfig m_hardConfig;
    NPCConfig m_classicConfig;

    void initPresets();
};

#endif // PRESETMANAGER_H
