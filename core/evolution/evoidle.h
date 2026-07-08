#ifndef EVOIDLE_H
#define EVOIDLE_H

#include "evolutionstate.h"

// 空闲状态：演化未启动
class EvoIdle : public EvolutionState {
public:
    void tick(AutoEvolutionEngine* engine) override;
    const char* name() const override { return "Idle"; }
};

#endif // EVOIDLE_H
