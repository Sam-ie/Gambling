#include "evofast.h"
#include "autoevolutionengine.h"
#include "evoidle.h"

void EvoFast::tick(AutoEvolutionEngine* engine)
{
    // 一轮全部交互
    engine->runner().runAllPairs(engine->npcs(), engine->history(), engine->payoff());

    engine->advanceRound();

    // 快速模式下每轮都通知UI更新
    engine->emitStep();
}
