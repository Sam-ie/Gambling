#ifndef EVOLUTIONSTATE_H
#define EVOLUTIONSTATE_H

#include <QVector>
#include "../npcscoreinfo.h"

class AutoEvolutionEngine;

// 演化状态抽象基类 —— 状态模式
// 每个具体状态封装一种演化运行模式的行为
class EvolutionState {
public:
    virtual ~EvolutionState() = default;

    // 进入状态时的初始化
    virtual void enter(AutoEvolutionEngine* engine) { Q_UNUSED(engine); }

    // 每次定时器触发时调用，执行一个工作单元
    virtual void tick(AutoEvolutionEngine* engine) = 0;

    // 状态名称（调试/日志用途）
    virtual const char* name() const = 0;
};

#endif // EVOLUTIONSTATE_H
