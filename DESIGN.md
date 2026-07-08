# 囚徒困境模拟器 — 设计文档

## 目录

1. [架构总览](#1-架构总览)
2. [目录与模块职责](#2-目录与模块职责)
3. [NPC 策略详解](#3-npc-策略详解)
4. [设计模式应用](#4-设计模式应用)
5. [游戏机制](#5-游戏机制)
6. [关键重构决策](#6-关键重构决策)
7. [扩展指南](#7-扩展指南)

---

## 1. 架构总览

### 分层设计

```
┌─────────────────────────────────────────────────┐
│                    UI Layer                      │
│  Gambling (主窗口)  ←→  NPCCircleWidget         │
│  SpinGroupController, PresetManager, HelpBuilder │
└──────────────┬──────────────────────────────────┘
               │ Qt Signal / Slot（单向数据流）
┌──────────────▼──────────────────────────────────┐
│              Core Layer (GameEngine)             │
│  PayoffMatrix  │  InteractionRunner              │
│  HistoryRecord │  EliminationManager             │
│  AutoEvolutionEngine (State Pattern × 4)        │
└──────────────┬──────────────────────────────────┘
               │
┌──────────────▼──────────────────────────────────┐
│            NPC Strategy Layer                    │
│  NPCBase (Strategy)  ←  11 strategy subclasses   │
│  NPCFactory (Factory)                            │
└─────────────────────────────────────────────────┘
```

### 核心原则

- **Core 不依赖 UI**：GameEngine 及其子模块是纯逻辑对象，不引用任何 UI 类
- **信号驱动更新**：Engine 通过 Qt 信号通知 UI 刷新，UI 通过 Engine 的公共方法执行操作
- **值对象隔离**：PayoffMatrix、NPCConfig、NPCScoreInfo 是纯数据类，零外部依赖

### GameEngine 状态机

```
IDLE ──startGame()──→ NPC_INTERACTION ──交互完成──→ WAITING_FOR_PLAYER
                       ↑                                    │
                       │                        playerAction(action)
                       │                                    │
                       └────── 回合未满 ─── advanceTurn() ──┘
                                                    │
                                          回合已满 → FINISHED
```

### 回合流程

每个回合分两步：

**第一步：NPC 内部博弈**。对所有 (NPC_i, NPC_j) 且 i < j，双方各调用 `action()` 出招 → 存入 HistoryRecord → 查支付矩阵分别加分 → 调用 `postRoundUpdate()` 供强化学习 NPC 更新内部状态。

**第二步：玩家对战**。对每个 NPC，发射 `playerTurn` 信号 → UI 等待用户点击诚实/欺骗 → 调用 Engine 的 `playerAction(action)` → NPC 调用 `action(玩家槽位, history)` → 查支付矩阵 → 发射 `turnResult` 信号。

### 信号清单

| 信号 | 触发时机 | UI 响应 |
|------|----------|---------|
| `gameStarted()` | 游戏开始 | 禁用开始按钮，启用诚实/欺骗按钮 |
| `roundChanged(r, t)` | 进入新回合 | 显示 "第 r/t 回合" |
| `playerTurn(id, name)` | 轮到与某 NPC 对战 | 显示对手信息，高亮对应圆 |
| `turnResult(...)` | 玩家与 NPC 交互完成 | 显示本轮结果 |
| `allScoresUpdated()` | 积分更新 | 刷新圆形面板积分 |
| `gameFinished(score)` | 游戏结束 | 显示最终排名 |
| `autoEvoStep(...)` | 演化每步推进 | 更新 UI 和连线 |
| `autoEvoFinished(...)` | 演化完成 | 显示策略分布 |

---

## 2. 目录与模块职责

```
Gambling/
├── main.cpp                         # QApplication 入口，全局样式/调色板
├── Gambling.pro                     # QMake 项目文件
│
├── core/                            # 核心引擎层（纯逻辑，无 UI 依赖）
│   ├── gameengine.h/.cpp            # 游戏引擎：生命周期 + 玩家交互 + 协调各组件
│   ├── historyrecord.h/.cpp         # 交互历史：N×N 矩阵记录所有博弈
│   ├── npcscoreinfo.h              # NPC 概要信息（值对象）
│   ├── payoffmatrix.h              # 支付矩阵（值对象）
│   ├── interactionrunner.h/.cpp    # 交互执行器：统一出牌→失误→记录→计分→RL流程
│   │
│   ├── npc/                        # NPC 策略子系统
│   │   ├── npcbase.h/.cpp          # 策略抽象基类
│   │   ├── npcfactory.h/.cpp       # 工厂模式，从 kTypeMetas[] 表派生所有映射
│   │   ├── npcconfig.h             # NPC 配置（Builder，替代 11 参数函数）
│   │   └── *.h                     # 11 个策略类（header-only）
│   │
│   ├── evolution/                  # 自动演化子系统
│   │   ├── evolutionstate.h        # 状态模式抽象基类
│   │   ├── evoidle.h/.cpp          # 空闲状态
│   │   ├── evopairbypair.h/.cpp    # 逐对演化（50ms/tick × 1对）
│   │   ├── evofast.h/.cpp          # 极速演化（50ms/tick × 整轮）
│   │   ├── evosingleround.h/.cpp   # 单轮演化（逐对跑完一轮自停）
│   │   └── autoevolutionengine.h/.cpp  # 演化引擎（管理状态切换）
│   │
│   └── elimination/                # 淘汰子系统
│       └── eliminationmanager.h/.cpp  # 淘汰 + 积分折算
│
├── ui/                             # UI 层
│   ├── gambling.h/.cpp             # 主窗口（精简后 ~500 行）
│   ├── gambling.ui                 # 主界面（Qt Designer）
│   ├── welcome.ui                  # 欢迎页
│   ├── npccirclewidget.h/.cpp      # 圆形可视化面板（QPainter 自绘）
│   ├── numberpicker.h/.cpp         # 移动端数字选择器（替代 QSpinBox）
│   ├── spingroupcontroller.h/.cpp  # Spin 控件组统一管理
│   ├── helptextbuilder.h/.cpp      # 帮助文本生成（中/英）
│   └── presetmanager.h/.cpp        # 预设配置（4 种难度）
│
└── Gambling_en.ts                  # 英文翻译源文件
```

### 依赖方向

```
UI (gambling, widgets)
  → SpinGroupController, PresetManager, HelpTextBuilder
  → Core (gameengine)
       → PayoffMatrix (独立值对象)
       → InteractionRunner
       → HistoryRecord
       → EliminationManager
       → AutoEvolutionEngine → EvolutionState (×4)
       → NPC Layer (npcbase, strategies, factory)
```

**规则**：Core 层不引用 UI 层的任何类型。NPC 层只依赖 HistoryRecord，不感知 Engine/Evolution。值对象零外部依赖。

---

## 3. NPC 策略详解

所有 NPC 继承自 `NPCBase`，核心接口：

```cpp
virtual int action(int opponentId, const HistoryRecord& history) = 0;
// 返回 0 = 合作, 1 = 背叛

virtual void postRoundUpdate(int opponentId, int myAction, int opponentAction,
                              int myScore, int opponentScore);
// 默认空实现，供强化学习者覆写
```

### 3.1 诚实者 (HonestNPC)

始终返回 0。无条件合作。是最容易被剥削的策略，但在合作型环境中也能获得稳定的互惠收益。

### 3.2 背叛者 (DeceptiveNPC)

始终返回 1。无条件背叛。单次博弈占优，面对 Tit-for-Tat 时长期亏损。

### 3.3 摇摆者 (SwingerNPC)

`rand() % 2`，完全随机。不可预测，代表混合策略均衡中的随机博弈者。

### 3.4 复读者 (RepeaterNPC) — Tit-for-Tat

第一轮默认为合作。之后复制对手上一轮对我的行为。

```
对手上轮合作 → 本轮合作
对手上轮背叛 → 本轮背叛
```

Anatol Rapoport 在 Axelrod 锦标赛中获胜的策略。四个关键特征：善良（先合作）、报复性（被背叛必反击）、宽容（对手恢复合作我立即恢复）、清晰（对手容易预测我的行为）。

### 3.5 宽恕者 (ForgivingNPC) — Tit-for-Two-Tats

复读者的宽容版本。对手连续背叛两次才反击。

```
对手连续两次背叛 → 背叛
否则 → 合作
```

### 3.6 强化学习者 (ReinforcementNPC) — ε-greedy Q-Learning

为每个对手独立维护统计数据：

```
OpponentStats {
    honestCount: 我选择合作的次数
    honestScore: 合作获得的总分
    cheatCount:  我选择背叛的次数
    cheatScore:  背叛获得的总分
}
```

决策：计算每次行为的平均收益，90% 概率选择平均收益最高的行为（贪婪），10% 概率随机探索（ε=0.1）。通过 `postRoundUpdate()` 在每轮结束后更新统计。首次遭遇统计为空，随机选择。

### 3.7 记仇者 (GrudgerNPC) — 扳机策略

一旦被某个对手背叛过，对该对手永久标记为不信任，此后永远背叛。对未背叛过的对手保持合作。这是 Friedman 的扳机策略（Trigger Strategy），威胁最严厉但失去合作机会后无法挽回。

### 3.8 试探者 (DetectiveNPC)

前四轮按固定模式试探：C → D → C → C。试探期结束后：
- 如果对手在试探期有过背叛 → 认为对手不可信，转为 Tit-for-Tat
- 如果对手从未背叛 → 认为对手太软弱，转为永远背叛（剥削）

Axelrod 锦标赛亚军策略，但需要足够多的回合才能发挥优势。

### 3.9 趋利者 (PavlovNPC) — Win-Stay Lose-Shift

根据上一轮结果决定本轮行为：

```
上一轮结果判据：双方行为相同（都合作或都背叛）→ "赢" → 保持行为不变
上一轮结果判据：双方行为不同（一合作一背叛）→ "输" → 切换行为
```

### 3.10 从众者 (MajorityNPC)

统计对手的整体行为倾向（合作次数 vs 背叛次数），跟随多数。平局时选择合作。

### 3.11 周期者 (PeriodicNPC)

固定模式循环：C → C → D → D → C → C → D → D → ...

---

## 4. 设计模式应用

### 4.1 策略模式 — NPC 行为子系统

每种 NPC 封装自己的决策算法。添加新策略只需继承 `NPCBase` 并实现 `action()`，无需修改任何现有代码（开闭原则）。

### 4.2 工厂模式 — NPC 创建

`NPCFactory::create()` 返回 `std::unique_ptr<NPCBase>`。单一元数据表 `kTypeMetas[]` 消除了类型名称映射的重复：

```cpp
struct TypeMeta {
    NPCFactory::NPCType type;
    const char* chineseName;
    const char* defaultPrefix;
};
```

`getTypeName()` 和 `getAllTypes()` 都从同一张表派生，不存在两份独立的数据源。

### 4.3 状态模式 — 自动演化引擎

重构前用 5 个布尔标志位管理 6 种状态（`m_autoEvoPaused`、`m_autoEvoAdjustingRound`、`m_autoEvoWaitingAdjust`、`m_autoEvoFastMode`、`m_autoEvoSingleRound`），组合爆炸且难以维护。

重构后每种运行模式封装为独立状态类：

```
EvoIdle ──start()──→ EvoPairByPair ──fastMode()──→ EvoFast
                        │    │
                        │    └── stepRound()
                        ▼
                   EvoSingleRound
```

每个状态的 `tick()` 只写自己模式的逻辑，不检查其他状态标志。

### 4.4 观察者模式 — Qt Signal/Slot

Engine 发信号，UI 订阅。数据流单向：Engine → 信号 → UI。UI 不直接操作 Engine 内部状态。

### 4.5 外观模式 — GameEngine

GameEngine 对外提供统一的游戏控制接口，对内委托给 InteractionRunner、EliminationManager、AutoEvolutionEngine 等子模块。`Gambling` 只通过 GameEngine 操作游戏，不感知内部模块划分。

### 4.6 Builder 模式 — NPC 配置

重构前 `initializeNPCs(11个int参数)` 参数列表冗长易错。重构后：

```cpp
NPCConfig config;
config.set(NPCFactory::HONEST, 3);
config.set(NPCFactory::REPEATER, 2);
m_engine->initializeNPCs(config);
```

`SpinGroupController::buildConfig()` 从 UI 控件自动构建配置，消除 11 参数的传递。

---

## 5. 游戏机制

### 5.1 支付矩阵

标准囚徒困境矩阵，用户可在设置页自定义参数：

|              | 对方合作 | 对方背叛 |
|--------------|----------|----------|
| **我方合作** | R, R     | S, T     |
| **我方背叛** | T, S     | P, P     |

默认值（Axelrod 参数）：R=1, T=2, S=-1, P=0。约束：T > R > P > S 且 2R > T + S。

### 5.2 淘汰机制

每隔 N 轮（用户可设），按积分排序，淘汰积分最低的若干个 NPC（数量由百分比参数控制），用当前积分最高 NPC 的同类型克隆体填补空缺。模拟自然选择，弱势策略逐渐被强势策略取代。

淘汰轮次有一个短暂的"展示回合"：不推进博弈，只显示真实积分供用户观察，然后执行折算（将真实积分按缩放规则转换为博弈内积分），再继续下一轮。

### 5.3 遗传策略

每回合结束时，按积分比例轮盘赌选取一个父代，将其类型的副本替换当前最低分个体。高分策略繁殖概率更高。

### 5.4 自动演化四种模式

| 模式 | 粒度 | 速度 | 连线 | 暂停 |
|------|------|------|------|------|
| 逐对演化 | 1 对/tick | 50ms | ✓ | 淘汰轮 300ms 停顿 |
| 极速演化 | 整轮/tick | 50ms | ✗ | 无停顿 |
| 步进 | 1 对/tick | 手动 | ✓ | 每步手动控制 |
| 单轮 | 1 对/tick | 50ms | ✓ | 每轮结束自停 |

### 5.5 失误率

每次 NPC 决策有 ε 概率忽略自身策略，随机选择合作或背叛。模拟现实中的不确定性（通信故障、误操作等）。玩家不受失误率影响。

### 5.6 HistoryRecord 数据结构

二维 Vector：`m_history[a][b]` 记录 a 对 b 的每一轮 (a行为, b行为)。双向写入保证对称性。玩家占用最后一个槽位（索引 = npcCount），NPC 可以通过索引查询与玩家的交互历史。

---

## 6. 关键重构决策

### 6.1 为什么状态模式优于标志位

5 个布尔值的组合会产生 32 种理论状态，其中只有 6 种合法。维护者需要记住所有合法组合，任何一处遗漏判断都可能引入 bug。状态模式将 6 种状态隔离为 6 个类，各自独立，互不干扰。

### 6.2 为什么 InteractionRunner 用成员方法而非静态函数

`InteractionRunner` 持有 `std::mt19937` 随机数引擎作为成员变量。静态函数共享全局随机数状态会导致不可复现的结果和潜在的线程安全问题。作为成员，每个 InteractionRunner 实例拥有独立的随机数状态。同时它封装了失误率参数，消除了三处重复的"出牌→失误判定→记录→计分→RL更新"流程。

### 6.3 为什么 NPC 策略保留 header-only

11 个策略类大多只有 20-60 行代码，核心逻辑集中在 `action()` 方法。拆分为 .h/.cpp 会增加文件数量却没有实质的编译收益或抽象收益。当前的 header-only 设计在代码清晰度和项目管理之间取得了平衡。

### 6.4 为什么 SpinGroupController 需要手动注册

Qt UI 文件中控件名称是固定的（如 `spinHonestCount`），类型映射必须在代码中建立。虽然在构造函数中写了 11 行 `registerSpin()` 调用，但所有后续操作（启用/禁用/取值/设值）都由 `SpinGroupController` 统一管理，消除了原本 5 处重复的 11 行代码块。

---

## 7. 扩展指南

### 添加新 NPC 策略

1. 在 `core/npc/` 创建头文件（如 `randomnpc.h`），继承 `NPCBase`，实现 `action()`
2. 在 `NPCFactory::NPCType` 枚举中添加新类型
3. 在 `kTypeMetas[]` 中添加元数据条目
4. 在 `NPCFactory::create()` 的 switch 中添加创建分支
5. 在 `NPCCircleWidget::initColors()` 中添加颜色映射
6. 在 UI 中添加对应的 NumberPicker 控件和信号绑定
7. 在 `HelpTextBuilder` 的策略列表中添加说明
8. 在 `PresetManager` 的预设数据中添加（可选）

### 添加新的自动演化模式

1. 创建新的状态类，继承 `EvolutionState`
2. 在 `AutoEvolutionEngine` 中添加对应的状态切换入口
3. 在 UI 中添加对应的按钮/触发方式

### 扩展为多方博弈（如公共品博弈）

1. 扩展 `NPCBase::action()` 接口支持多名对手参数
2. 扩展 `InteractionRunner::run()` 支持多人交互
3. 扩展 `HistoryRecord` 支持多人记录格式
4. `PayoffMatrix` 保持独立，可替换为不同的收益计算逻辑

