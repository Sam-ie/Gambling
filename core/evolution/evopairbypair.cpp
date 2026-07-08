#include "evopairbypair.h"
#include "autoevolutionengine.h"
#include "evoidle.h"

void EvoPairByPair::tick(AutoEvolutionEngine* engine)
{
    int n = engine->npcCount();
    if (n < 2) return;

    int i = engine->pairI();
    int j = engine->pairJ();

    // 跳过无效索引
    while (i < n && j >= n) {
        i++;
        j = i + 1;
    }

    if (i >= n || j >= n) {
        engine->setPairIndex(0, 1);
        engine->advanceRound();
        return;
    }

    // 保存当前对的 ID（在索引推进之前）
    int idA = engine->npcAt(i)->getId();
    int idB = engine->npcAt(j)->getId();

    // 执行交互
    engine->runner().run(*engine->npcAt(i), *engine->npcAt(j),
                         engine->history(), engine->payoff());

    // 推进到下一对
    j++;
    if (j >= n) { i++; j = i + 1; }
    engine->setPairIndex(i, j);

    engine->emitStep();
    engine->emitPairDone(idA, idB);
}
