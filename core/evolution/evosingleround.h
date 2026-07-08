#ifndef EVOSINGLEROUND_H
#define EVOSINGLEROUND_H

#include "evolutionstate.h"

// 单轮演化状态：逐对执行一整轮后自动停止
class EvoSingleRound : public EvolutionState {
public:
    void tick(AutoEvolutionEngine* engine) override;
    const char* name() const override { return "SingleRound"; }
};

#endif // EVOSINGLEROUND_H
