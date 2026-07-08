#include "evosingleround.h"
#include "autoevolutionengine.h"
#include "evoidle.h"

void EvoSingleRound::tick(AutoEvolutionEngine* engine)
{
    int n = engine->npcCount();
    if (n < 2) return;

    int i = engine->pairI();
    int j = engine->pairJ();

    while (i < n && j >= n) {
        i++;
        j = i + 1;
    }

    if (i >= n || j >= n) {
        engine->setPairIndex(0, 1);
        engine->advanceRound();
        if (engine->isRunning()) {
            engine->transitionTo(new EvoIdle());
        }
        return;
    }

    int idA = engine->npcAt(i)->getId();
    int idB = engine->npcAt(j)->getId();

    engine->runner().run(*engine->npcAt(i), *engine->npcAt(j),
                         engine->history(), engine->payoff());

    j++;
    if (j >= n) { i++; j = i + 1; }
    engine->setPairIndex(i, j);

    engine->emitStep();
    engine->emitPairDone(idA, idB);
}
