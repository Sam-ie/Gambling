#ifndef EVOPAIRBYPAIR_H
#define EVOPAIRBYPAIR_H

#include "evolutionstate.h"

// 逐对演化状态：每个定时器 tick 执行一对 NPC 交互
// 一轮全部完成后自动推进回合，处理淘汰/折算
class EvoPairByPair : public EvolutionState {
public:
    void tick(AutoEvolutionEngine* engine) override;
    const char* name() const override { return "PairByPair"; }
};

#endif // EVOPAIRBYPAIR_H
