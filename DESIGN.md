# Gambling 项目设计文档

## 项目概述

**Gambling** 是一个基于囚徒困境（Prisoner's Dilemma）的多策略锦标赛模拟器。支持玩家与 11 种 AI 策略进行对抗，以及全自动演化（遗传算法驱动的策略淘汰赛）。技术栈：C++17 + Qt 6，支持桌面端和 Android 端。

---

## 博弈论背景

### 囚徒困境标准模型

|          | 对方合作 | 对方背叛 |
|----------|----------|----------|
| **我方合作** | R, R     | S, T     |
| **我方背叛** | T, S     | P, P     |

约束条件：`T > R > P > S` 且 `2R > T + S`

默认参数（标准 Axelrod 参数）：R=3, T=5, S=0, P=1

### 11 种 AI 策略

| 策略 | 英文名 | 核心逻辑 | 理论来源 |
|------|--------|----------|----------|
| 诚实者 | Honest | 始终合作 | 无条件合作 |
| 背叛者 | Deceptive | 始终背叛 | 无条件背叛 |
| 摇摆者 | Swinger | 50% 随机 | 随机策略 |
| 复读者 | Repeater | Tit-for-Tat | Anatol Rapoport, 1980 |
| 宽恕者 | Forgiving | Tit-for-Two-Tats | 宽容版 TFT |
| 强化学习者 | Reinforcement | ε-greedy Q-Learning | Sutton & Barto |
| 记仇者 | Grudger | 一次背叛永久记仇 | Friedman 扳机策略 |
| 试探者 | Detective | 4 轮试探 + 条件策略 | Axelrod 锦标赛亚军 |
| 趋利者 | Pavlov | Win-Stay Lose-Shift | Nowak & Sigmund, 1993 |
| 从众者 | Majority | 跟随对手多数行为 | 从众行为 |
| 周期者 | Periodic | 固定 CDDC 周期 | 周期策略 |

---

## 架构设计

### 目录结构

```
Gambling/
├── core/                          # 核心引擎层（纯逻辑，无 UI 依赖）
│   ├── gameengine.h/.cpp          # 游戏引擎：生命周期 + 玩家交互 + 协调各组件
│   ├── historyrecord.h/.cpp       # 交互历史：N×N 矩阵记录
│   ├── npcscoreinfo.h             # NPC 概要信息（值对象）
│   ├── payoffmatrix.h             # 支付矩阵（值对象）
│   ├── interactionrunner.h/.cpp   # 交互执行器
│   │
│   ├── npc/                       # NPC 策略子系统
│   │   ├── npcbase.h/.cpp         # 策略抽象基类
│   │   ├── npcfactory.h/.cpp      # 工厂模式
│   │   ├── npcconfig.h            # NPC 配置（值对象）
│   │   └── *.h                    # 11 个具体策略类
│   │
│   ├── evolution/                 # 自动演化子系统
│   │   ├── evolutionstate.h       # 状态抽象基类
│   │   ├── evoidle.h/.cpp         # 空闲状态
│   │   ├── evopairbypair.h/.cpp   # 逐对演化状态
│   │   ├── evofast.h/.cpp         # 极速演化状态
│   │   ├── evosingleround.h/.cpp  # 单轮演化状态
│   │   └── autoevolutionengine.h/.cpp  # 演化引擎
│   │
│   └── elimination/               # 淘汰子系统
│       └── eliminationmanager.h/.cpp
│
├── ui/                            # UI 层
│   ├── gambling.h/.cpp            # 主窗口（精简后 ~500 行）
│   ├── gambling.ui                # 主界面
│   ├── welcome.ui                 # 欢迎页
│   ├── npccirclewidget.h/.cpp     # 圆形可视化组件
│   ├── numberpicker.h/.cpp        # 移动端数字选择器
│   ├── spingroupcontroller.h/.cpp # Spin 控件组管理
│   ├── helptextbuilder.h/.cpp     # 帮助文本生成
│   └── presetmanager.h/.cpp       # 预设配置管理
│
└── main.cpp                       # 入口
```

### 架构总览图

```
┌─────────────────────────────────────────────────┐
│                    UI Layer                      │
│  ┌───────────────┐  ┌──────────────────────┐    │
│  │ Gambling      │  │ NPCCircleWidget      │    │
│  │ (主窗口)      │  │ (圆形可视化)          │    │
│  │ ┌───────────┐ │  └──────────────────────┘    │
│  │ │SpinGroup  │ │  ┌──────────────────────┐    │
│  │ │Controller │ │  │ NumberPicker         │    │
│  │ └───────────┘ │  └──────────────────────┘    │
│  │ ┌───────────┐ │                              │
│  │ │Preset     │ │  ┌──────────────────────┐    │
│  │ │Manager    │ │  │ HelpTextBuilder      │    │
│  │ └───────────┘ │  └──────────────────────┘    │
│  └───────┬───────┘                              │
└──────────┼──────────────────────────────────────┘
           │ signals/slots (Observer Pattern)
┌──────────┼──────────────────────────────────────┐
│          ▼          Core Layer                  │
│  ┌─────────────────────────────────────────┐    │
│  │           GameEngine (Facade)             │    │
│  │  ┌───────────┐  ┌────────────────────┐   │    │
│  │  │PayoffMatrix│  │InteractionRunner   │   │    │
│  │  └───────────┘  └────────────────────┘   │    │
│  │  ┌───────────┐  ┌────────────────────┐   │    │
│  │  │Elimination│  │AutoEvolution       │   │    │
│  │  │Manager    │  │Engine (State)      │   │    │
│  │  └───────────┘  └────────────────────┘   │    │
│  │  ┌───────────┐                           │    │
│  │  │HistoryRec │                           │    │
│  │  └───────────┘                           │    │
│  └──────────────┬──────────────────────────┘    │
│                 │                                │
│  ┌──────────────┴──────────────────────────┐    │
│  │           NPC Strategy Layer              │    │
│  │  ┌──────────────────────────────────┐    │    │
│  │  │        NPCBase (Strategy)         │    │    │
│  │  │  ┌──┬──┬──┬──┬──┬──┬──┬──┬──┐   │    │    │
│  │  │  │H │D │S │R │F │R │G │D │P │..│   │    │    │
│  │  │  └──┴──┴──┴──┴──┴──┴──┴──┴──┴──┘   │    │    │
│  │  │       NPCFactory (Factory)          │    │    │
│  │  └──────────────────────────────────┘    │    │
│  └──────────────────────────────────────────┘    │
└──────────────────────────────────────────────────┘
```

---

## 设计模式应用

### 1. 策略模式 (Strategy Pattern) —— NPC 行为子系统

**位置**: `core/npc/npcbase.h` + 11 个策略类

**问题**: 每种 NPC 有不同的决策算法。如果不用策略模式，就需要一个庞大的 if-else 链或 switch-case 来判断 NPC 类型。

**解决方案**: 定义抽象接口 `NPCBase::action()`，每个具体策略封装自己的算法。

```cpp
class NPCBase {
public:
    virtual int action(int opponentId, const HistoryRecord& history) = 0;
    virtual void postRoundUpdate(...) { /* 默认空实现 */ }
};

class RepeaterNPC : public NPCBase {
    int action(int opponentId, const HistoryRecord& history) override {
        auto actions = history.getActionsAgainst(opponentId, m_id);
        return actions.isEmpty() ? 0 : actions.last(); // Tit-for-Tat
    }
};
```

**扩展性**: 添加新策略只需继承 `NPCBase`，无需修改任何现有代码（开闭原则）。

---

### 2. 工厂模式 (Factory Pattern) —— NPC 创建

**位置**: `core/npc/npcfactory.h/.cpp`

**问题**: 需要根据枚举类型创建对应的策略实例，并返回合适的生命周期管理方式。

**解决方案**:
- `NPCFactory::create()` 返回 `std::unique_ptr<NPCBase>`，明确所有权转移
- 单一元数据表 `kTypeMetas[]` 消除类型名称映射的重复定义

```cpp
struct TypeMeta {
    NPCFactory::NPCType type;
    const char* chineseName;      // 中文名（UI 显示）
    const char* defaultPrefix;    // 默认名称前缀（NPC 命名）
};

static const TypeMeta kTypeMetas[] = { ... };
// getTypeName() 和 getAllTypes() 都从此表派生
```

---

### 3. 状态模式 (State Pattern) —— 自动演化引擎

**位置**: `core/evolution/evolutionstate.h` + 4 个状态类

**问题**: 原代码用 5 个布尔标志位管理 6 种状态，状态转换逻辑分散在多处：

```cpp
// 重构前：混乱的标志位
bool m_autoEvoPaused;
bool m_autoEvoAdjustingRound;
bool m_autoEvoWaitingAdjust;
bool m_autoEvoFastMode;
bool m_autoEvoSingleRound;
```

**解决方案**: 每个演化模式封装为一个状态类，状态转换由 `AutoEvolutionEngine` 统一管理。

```
         ┌──────────┐
         │ EvoIdle  │◄──────────────────────┐
         └────┬─────┘                       │
              │ start()                     │ stop()/finish
              ▼                             │
    ┌─────────────────┐                     │
    │ EvoPairByPair   │─────────────────────┘
    └────┬───────┬────┘
         │       │ fastMode()
         │       ▼
         │  ┌─────────┐
         │  │ EvoFast │─────────────────────┐
         │  └─────────┘                     │
         │                                  │
         │ stepRound()                      │
         ▼                                  │
    ┌──────────────┐                        │
    │EvoSingleRound│────────────────────────┘
    └──────────────┘
```

**关键优势**: 每个状态的 tick() 方法只关注自己的运行逻辑，无需检查其他状态标志。添加新的运行模式只需新增一个状态类。

---

### 4. 观察者模式 (Observer Pattern) —— Qt Signal/Slot

**位置**: `GameEngine` / `AutoEvolutionEngine` 信号 → `Gambling` 槽

**问题**: Engine 层不应直接操作 UI，UI 层应响应 Engine 的状态变化。

**解决方案**: Qt 原生 signal/slot 机制。Engine 发出信号，UI 通过 connect 订阅。

```
GameEngine::roundChanged(int, int)  ──→ Gambling::onRoundChanged()
GameEngine::playerTurn(...)         ──→ Gambling::onPlayerTurn()
GameEngine::autoEvoStep(...)        ──→ Gambling::onAutoEvoStep()
AutoEvolutionEngine::pairDone(...)  ──→ NPCCircleWidget::highlightPair()
```

数据流单向：Engine → 信号 → UI。UI 通过 Engine 的公共方法调用执行操作。

---

### 5. 外观模式 (Facade Pattern) —— GameEngine

**位置**: `core/gameengine.h/.cpp`

**问题**: 重构前 GameEngine 承载了 619 行代码，混合了 NPC 管理、交互执行、淘汰、演化等所有逻辑。

**解决方案**: GameEngine 作为外观，对外提供统一的游戏控制接口，对内委托各子模块：

```cpp
class GameEngine : public QObject {
    PayoffMatrix        m_payoff;       // 支付矩阵计算
    InteractionRunner   m_runner;       // 单次交互执行
    HistoryRecord       m_history;      // 交互历史
    EliminationManager  m_eliminator;   // 淘汰 + 积分折算
    AutoEvolutionEngine m_evoEngine;    // 自动演化状态机
    
    std::vector<std::unique_ptr<NPCBase>> m_npcs;  // NPC 容器
};
```

`Gambling` 通过 GameEngine 的公共接口操作游戏，不感知内部模块划分。

---

### 6. Builder 模式 —— NPC 配置

**位置**: `core/npc/npcconfig.h`

**问题**: 原 `initializeNPCs(11个int参数)` 参数列表过长、易错。

**解决方案**: `NPCConfig` 类提供类型安全的配置：

```cpp
NPCConfig config;
config.set(NPCFactory::HONEST, 3);
config.set(NPCFactory::REPEATER, 2);
// config.forEach() 遍历非零条目
m_engine->initializeNPCs(config);
```

`SpinGroupController::buildConfig()` 从 UI 控件组自动构建配置，消除了 11 个参数的传递。

---

## 模块职责与依赖

### 依赖方向（从上到下）

```
UI (gambling, widgets)
  │
  ├─ SpinGroupController
  ├─ PresetManager
  ├─ HelpTextBuilder
  │
  ▼
Core (gameengine) ── Facade
  │
  ├─ PayoffMatrix     (独立值对象)
  ├─ InteractionRunner (独立执行器)
  ├─ HistoryRecord    (独立存储)
  ├─ EliminationManager (独立子系统)
  └─ AutoEvolutionEngine (独立子系统)
       └─ EvolutionState (×4)
  │
  ▼
NPC Layer (npcbase, strategies, factory)
```

**规则**:
- Core 层不依赖 UI 层
- NPC 层不依赖 Core 层的 Engine/Evolution，只依赖 HistoryRecord
- 值对象（PayoffMatrix、NPCConfig、NPCScoreInfo）无外部依赖

---

## 关键重构决策

### 1. 为什么用状态模式而非简单的 if-else？

自动演化有 4 种运行模式，各自有不同的"单步工作单元"：
- 逐对模式：每 tick 交互 1 对
- 极速模式：每 tick 交互全轮
- 单轮模式：逐对执行 1 轮后停止
- 单对步进：手动触发 1 对

如果用标志位，每个 tick 需要判断 5 个布尔值的组合（共 32 种可能状态）。状态模式将每种模式隔离为独立类，每个类的 tick() 方法只写自己模式的逻辑。

### 2. 为什么 InteractionRunner 用成员方法而非静态函数？

`InteractionRunner` 持有 `std::mt19937` 随机数引擎，避免静态变量带来的线程安全问题和不可复现性。同时它封装了失误率（errorRate）参数，统一了原本在 3 处重复的"出牌→失误判定→记录→计分→RL更新"流程。

### 3. 为什么 NPC 策略保留 header-only？

11 个策略类大多只有 20-60 行，且核心只有一个 `action()` 方法。拆成 .h/.cpp 会增加文件数量和编译时间却没有实质收益。当前的 header-only 设计在清晰度和实用性之间取得了平衡。

### 4. 为什么 SpinGroupController 需要 11 个 registerSpin() 调用？

Qt UI 文件中控件名称是固定的（如 `spinHonestCount`），类型映射需要手动建立。虽然在构造函数写 11 行注册代码，但后续所有 spin 操作（启用/禁用/取值/设值）都由 `SpinGroupController` 统一管理，消除了原先 5 处重复的 11 行代码块。

---

## 扩展指南

### 添加新 NPC 策略

1. 在 `core/npc/` 创建新头文件（如 `randomnpc.h`），继承 `NPCBase`
2. 在 `NPCFactory::NPCType` 枚举中添加新类型
3. 在 `kTypeMetas[]` 中添加元数据条目
4. 在 `NPCFactory::create()` 的 switch 中添加创建分支
5. 在 `NPCCircleWidget::initColors()` 中添加颜色映射
6. 在 `Gambling` UI 中添加对应的 NumberPicker 控件和信号绑定
7. 在 `HelpTextBuilder` 的策略列表中添加入口

### 添加新游戏机制

当前架构支持两方博弈（囚徒困境）。如要扩展为多方博弈（公共品博弈），需要：
1. 扩展 `NPCBase::action()` 接口支持多名对手参数
2. 扩展 `InteractionRunner::run()` 支持多人交互
3. 扩展 `HistoryRecord` 支持多人记录格式

`PayoffMatrix` 保持独立，可替换为不同的收益计算逻辑。

### 添加 NPC 行为修饰（buff/debuff）

利用装饰器模式：
```cpp
class AngryDecorator : public NPCBehaviorDecorator {
    int action(int opponentId, const HistoryRecord& history) override {
        // 愤怒时 80% 概率背叛
        return std::rand() % 100 < 80 ? 1 : m_wrapped->action(opponentId, history);
    }
};
```

---

## 重构前后对比

| 维度 | 重构前 | 重构后 |
|------|--------|--------|
| GameEngine 行数 | 619 行 | ~200 行 |
| Gambling 行数 | 916 行 | ~500 行 |
| 自动演化状态管理 | 5 个布尔标志位 | 4 个状态类 |
| 重复代码（spin 操作） | 5 处 × 11 行 | 0（SpinGroupController） |
| 重复代码（交互执行） | 3 处 | 0（InteractionRunner） |
| NPC 创建内存管理 | 裸指针 + 手动 delete | std::unique_ptr |
| 随机数引擎 | std::rand() + std::srand() | std::mt19937 |
| 类型映射 | 两份重复的 QMap | 单一 kTypeMetas[] 表 |
| 模块耦合度 | Engine + UI 双 God Class | 7 个独立子模块 |
| 可测试性 | 几乎不可单元测试 | 每个子模块可独立测试 |
