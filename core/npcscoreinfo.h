#ifndef NPCSCOREINFO_H
#define NPCSCOREINFO_H

#include <QString>

// 单个 NPC 的概要信息（供 UI 展示和信号传递）
struct NPCScoreInfo {
    int id = 0;
    QString name;
    QString strategyType;
    int score = 0;
};

#endif // NPCSCOREINFO_H
