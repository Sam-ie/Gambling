#ifndef AUTOEVOLUTIONENGINE_H
#define AUTOEVOLUTIONENGINE_H

#include <QObject>
#include <QTimer>
#include <memory>
#include <vector>

#include "npcbase.h"
#include "npcfactory.h"
#include "historyrecord.h"
#include "payoffmatrix.h"
#include "interactionrunner.h"
#include "../elimination/eliminationmanager.h"
#include "evolutionstate.h"
#include "../npcscoreinfo.h"

// 自动演化引擎 —— 状态模式 + 观察者模式
// 将 GameEngine 的演化逻辑完全分离，通过状态机管理 4 种运行模式
class AutoEvolutionEngine : public QObject
{
    Q_OBJECT

public:
    explicit AutoEvolutionEngine(QObject* parent = nullptr);
    ~AutoEvolutionEngine() override;

    // ---- 生命周期 ----
    void start(int totalRounds, bool startTimer = true);
    void stop();

    void pause();
    void resume();
    void stepPair();   // 手动单对步进
    void stepRound();   // 手动单轮步进
    void fastMode();    // 切换到极速模式

    bool isRunning() const { return m_running; }
    bool isPaused() const { return m_paused; }

    // ---- 状态转换 ----
    void transitionTo(EvolutionState* newState);

    // ---- 被 State 调用的上下文方法 ----
    std::vector<std::unique_ptr<NPCBase>>& npcs()          { return *m_npcs; }
    std::vector<NPCFactory::NPCType>&    npcTypes()        { return *m_npcTypes; }
    HistoryRecord&       history()                          { return *m_history; }
    InteractionRunner&   runner()                           { return *m_runner; }
    PayoffMatrix&        payoff()                           { return *m_payoff; }
    EliminationManager&  eliminator()                       { return *m_eliminator; }
    int&                 nextNpcId()                        { return *m_nextNpcId; }

    NPCBase* npcAt(int idx) const { return (*m_npcs)[idx].get(); }
    int npcCount() const { return static_cast<int>(m_npcs->size()); }

    int pairI() const { return m_pairI; }
    int pairJ() const { return m_pairJ; }
    void setPairIndex(int i, int j) { m_pairI = i; m_pairJ = j; }

    int currentRound()  const { return m_currentRound; }
    int targetRound()   const { return m_targetRound; }

    // 由状态调用：推进回合（含淘汰/折算）
    void advanceRound();

    void emitStep();
    void emitPairDone(int a, int b);

    // ---- 配置 ----
    void setErrorRate(double rate) { m_runner->setErrorRate(rate); }
    void setAutoEvoInterval(int ms) { m_timerInterval = ms; }

    // ---- 设置共享数据指针（GameEngine 委托时调用） ----
    void bind(std::vector<std::unique_ptr<NPCBase>>* npcs,
              std::vector<NPCFactory::NPCType>* types,
              HistoryRecord* history,
              InteractionRunner* runner,
              PayoffMatrix* payoff,
              EliminationManager* eliminator,
              int* nextNpcId);

signals:
    void step(int round, int totalRounds, const QVector<NPCScoreInfo>& rankings);
    void pairDone(int npcIdA, int npcIdB);
    void finished(const QVector<NPCScoreInfo>& finalRankings);

private:
    QTimer* m_timer = nullptr;
    std::unique_ptr<EvolutionState> m_state;

    // 共享数据（指向 GameEngine 的成员，不拥有）
    std::vector<std::unique_ptr<NPCBase>>* m_npcs = nullptr;
    std::vector<NPCFactory::NPCType>*      m_npcTypes = nullptr;
    HistoryRecord*       m_history = nullptr;
    InteractionRunner*   m_runner = nullptr;
    PayoffMatrix*        m_payoff = nullptr;
    EliminationManager*  m_eliminator = nullptr;
    int*                 m_nextNpcId = nullptr;

    // 演化状态
    bool m_running = false;
    bool m_paused = false;
    int m_currentRound = 0;
    int m_targetRound = 0;
    int m_pairI = 0;
    int m_pairJ = 1;
    int m_timerInterval = 100;

    QVector<NPCScoreInfo> buildRankings() const;
};

#endif // AUTOEVOLUTIONENGINE_H
