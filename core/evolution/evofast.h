#ifndef EVOFAST_H
#define EVOFAST_H

#include "evolutionstate.h"

// 极速演化状态：每个 tick 执行一整轮全部交互（不逐对显示）
class EvoFast : public EvolutionState {
public:
    void tick(AutoEvolutionEngine* engine) override;
    const char* name() const override { return "Fast"; }
};

#endif // EVOFAST_H
