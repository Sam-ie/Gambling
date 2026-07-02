# 囚徒困境博弈模拟器 — 设计文档

## 目录

1. [项目架构](#1-项目架构)
2. [目录结构](#2-目录结构)
3. [NPC 策略详解](#3-npc-策略详解)
4. [逻辑层设计](#4-逻辑层设计)
5. [支付矩阵](#5-支付矩阵)
6. [演化机制](#6-演化机制)

---

## 1. 项目架构

```
┌─────────────────────────────┐
│          main.cpp           │  入口
└─────────────┬───────────────┘
              │
    ┌─────────▼─────────┐
    │   ui/Gambling     │  视图层：窗口、按钮、文本显示
    │   (QMainWindow)   │  只负责 UI，不包含游戏逻辑
    └────────┬──────────┘
             │ 持有 + 信号连接
    ┌────────▼──────────┐
    │  core/GameEngine  │  逻辑层：所有游戏状态与规则
    │    (QObject)      │  通过信号→槽驱动 UI 更新
    └────────┬──────────┘
             │ 管理
    ┌────────┴──────────┐
    │  core/npc/        │  NPC 子系统
    │  NPCBase (抽象)   │  6 种策略子类
    │  NPCFactory       │  工厂创建
    └───────────────────┘
             │ 写入/查询
    ┌────────▼──────────┐
    │ core/HistoryRecord│  交互历史存储
    └───────────────────┘
```

**设计原则**：模型-视图分离。`GameEngine` 是纯逻辑对象，不引用任何 UI 类。UI 通过信号被动更新。

---

## 2. 目录结构

```
Gambling/
├── main.cpp                    # QApplication 入口
├── Gambling.pro                # QMake 项目文件
├── DESIGN.md                   # 本文档
│
├── core/                       # 逻辑层
│   ├── gameengine.h/.cpp       # 游戏引擎
│   ├── historyrecord.h/.cpp    # 交互历史记录
│   └── npc/                    # NPC 子系统
│       ├── npcbase.h/.cpp      # NPC 抽象基类
│       ├── npcfactory.h/.cpp   # NPC 工厂
│       ├── honestnpc.h         # 诚实者
│       ├── deceptivenpc.h      # 背叛者
│       ├── swingernpc.h        # 摇摆者
│       ├── repeaternpc.h       # 复读者
│       ├── delayedrepeaternpc.h # 延迟复读者
│       └── reinforcementnpc.h  # 强化学习者
│
└── ui/                         # 视图层
    ├── gambling.h/.cpp         # 主窗口
    └── gambling.ui             # Qt Designer 界面
```

QMake 通过 `INCLUDEPATH += core core/npc ui` 使各层之间通过简洁的相对路径引用：

```cpp
// ui/gambling.h
#include "gameengine.h"   // 在 core/ 中找到

// core/gameengine.h
#include "npcbase.h"      // 在 core/npc/ 中找到
#include "npcfactory.h"
```

---

## 3. NPC 策略详解

所有 NPC 继承自抽象的 `NPCBase`，核心接口为：

```cpp
virtual int action(int opponentId, const HistoryRecord& history) = 0;
// 返回 0 = 诚实（合作）, 1 = 欺骗（背叛）
```

### 3.1 诚实者 (HonestNPC)

**策略**：始终返回 0（合作）。

无论对手是谁、历史如何，永远选择合作。是最容易被利用的策略。

```cpp
int action(int opponentId, const HistoryRecord& history) override {
    return 0;
}
```

### 3.2 背叛者 (DeceptiveNPC)

**策略**：始终返回 1（欺骗）。

无条件背叛。在单次博弈中占优，但面对以牙还牙策略时长期亏损。

### 3.3 摇摆者 (SwingerNPC)

**策略**：`rand() % 2`，50/50 随机选择。

不可预测，不依赖历史。在混合策略均衡中代表完全随机化的博弈者。

### 3.4 复读者 (RepeaterNPC) — 以牙还牙

**策略**：复制对手上一轮对自己的行为。

```
第一轮：默认诚实（合作）
之后：对手上一轮怎么对我，我这轮就怎么对他
```

查询 `history.getActionsAgainst(opponentId, m_id)` 获取对手对我的历史行为序列，取最后一条。

这是 Axelrod 囚徒困境竞赛中表现最优的简单策略——它善良（先合作）、报复性（被背叛则反击）、宽容（对手恢复合作后立即恢复合作）。

### 3.5 延迟复读者 (DelayedRepeaterNPC)

**策略**：延迟一回合复制。

```
前两轮：默认诚实（合作）
之后：复制对手倒数第二轮的行为
```

查询 `history.getActionsAgainst(opponentId, m_id)`，取 `size() - 2` 位置的元素。相比复读者有更强的记忆力，对间歇性欺骗更宽容。

### 3.6 强化学习者 (ReinforcementNPC)

**策略**：ε-greedy + 经验统计。

为**每个对手**分别维护统计数据：

```
OpponentStats {
    honestCount: 诚实次数
    honestScore: 诚实总收益
    cheatCount:  欺骗次数
    cheatScore:  欺骗总收益
}
```

**决策时**：
1. 计算每次行为的平均收益：`honestAvg = honestScore / honestCount`，`cheatAvg = cheatScore / cheatCount`
2. 90% 概率贪婪选择（选平均收益高的行为）
3. 10% 概率随机探索（ε = 0.1）

**学习时**：每轮交互后，通过 `postRoundUpdate()` 回调更新统计数据。

首次遭遇时统计为空，随机选择。策略会随着回合数增加逐步逼近该对手的最优应对方案。

---

## 4. 逻辑层设计

### 4.1 GameEngine — 状态机驱动

`GameEngine` 是一个有限状态机，内部阶段（Phase）流转如下：

```
IDLE ──startGame()──→ NPC_INTERACTION ──NPC间交互完成──→ WAITING_FOR_PLAYER
                       ↑                                      │
                       │                          playerAction() 被调用
                       │                                      │
                       └────────── 回合未满 ──── advanceTurn() ─┘
                                                    │
                                          回合已满 → FINISHED
```

**各阶段行为**：

| 阶段 | 行为 | 触发条件 |
|------|------|----------|
| `IDLE` | 等待开始 | 初始 / 重置后 |
| `NPC_INTERACTION` | NPC 两两交互 | startGame / 新回合 |
| `WAITING_FOR_PLAYER` | 等待玩家按钮点击 | NPC 交互完成后 |
| `FINISHED` | 显示结果 | 回合数达到上限 |

### 4.2 信号驱动更新

`GameEngine` 通过 Qt 信号通知 `Gambling`（视图层）更新界面：

```
gameStarted()        → UI 禁用开始按钮，启用诚实/欺骗按钮
roundChanged(r,t)    → UI 显示"第 r/t 回合"
playerTurn(id,name)  → UI 显示当前对手信息
turnResult(...)      → UI 显示本轮结果
allScoresUpdated()   → UI 刷新积分显示
gameFinished(score)  → UI 显示最终排名
autoEvoStep(...)     → UI 更新演化进度
autoEvoFinished(...) → UI 显示演化结果
```

`Gambling` 只做视图转发：按钮点击 → 调用 `GameEngine::playerAction(0或1)`，其余一切由信号驱动。

### 4.3 回合流程

每个回合分两步：

**第一步：NPC 内部博弈**

```
对每对 (NPC_i, NPC_j)，i < j：
  1. action_i = NPC_i.action(NPC_j 的 ID, history)
  2. action_j = NPC_j.action(NPC_i 的 ID, history)
  3. 存入 HistoryRecord
  4. 查支付矩阵 → 分别加分
  5. 调用 postRoundUpdate() 供强化学习 NPC 更新
```

**第二步：玩家对战**

```
对每个 NPC：
  1. 发射 playerTurn 信号 → UI 等待用户点击
  2. 用户点击诚实/欺骗 → 调用 playerAction(action)
  3. NPC.action(玩家槽位, history) → 对手行为
  4. 查支付矩阵 → 双方加分
  5. 发射 turnResult 信号 → UI 显示结果
```

### 4.4 HistoryRecord

存储所有参与者（N 个 NPC + 1 个玩家槽位）之间的交互历史。

数据结构：二维 `QVector`，`m_history[a][b]` = `QVector<InteractionPair>`，记录 a 对 b 的每轮 (a行为, b行为)。

关键方法：

```
recordInteraction(a, b, actionA, actionB)
  → 同时写入 m_history[a][b] 和 m_history[b][a]（对称）

getActionsAgainst(a, b)
  → 返回 a 对 b 的所有历史行为序列

getInteractionHistory(a, b)
  → 返回 a 与 b 的完整交互对列表
```

玩家槽位 = `npcCount`（排在最后一个索引），这样 NPC 可以通过玩家槽位的 ID 查询与玩家的交互历史。

---

## 5. 支付矩阵

标准囚徒困境矩阵：

|                    | 对手诚实 | 对手欺骗 |
|--------------------|----------|----------|
| **你诚实**          | (2, 2)   | (0, 3)   |
| **你欺骗**          | (3, 0)   | (1, 1)   |

格式：(你的得分, 对手的得分)

- 双方合作收益最高（总计 4 分）
- 单方面背叛收益最大（3 分），但对方得 0
- 双方背叛陷入囚徒困境（各 1 分）

---

## 6. 演化机制

### 6.1 淘汰策略

每隔 N 回合（用户可设），淘汰当前积分最低的 NPC，克隆积分最高的 NPC（同类型、新名称）。

```
applyElimination():
  1. 遍历所有 NPC，找到最高分和最低分
  2. 删除最低分 NPC
  3. 用最高分 NPC 的类型创建一个新 NPC 替换
```

效果：弱势策略逐渐被强势策略取代，模拟自然选择。

### 6.2 遗传策略

每回合结束时，按积分比例随机选择一个"父代"，将其类型的副本替换当前最低分个体。

```
applyGenetics():
  1. 计算所有 NPC 的总积分（经偏移确保非负）
  2. 按积分权重随机抽样 → 选出一个父代
  3. 找到当前最低分个体
  4. 用父代类型的副本替换最低分个体
```

效果：高分策略有更高概率繁衍，类似"适者生存"的遗传算法。

### 6.3 自动演化

无需玩家参与，纯 NPC 之间运行 N 回合（用户设定 10~1000 回合）。通过 `QTimer` 分步执行，每步约 200ms，保持 UI 响应。

```
startAutoEvolution(totalRounds, ...):
  1. 创建 QTimer（每 200ms 触发一次）
  2. 每次触发 autoEvoTick()：
     a. 所有 NPC 两两交互
     b. 结算积分
     c. 执行淘汰/遗传（如果开启）
     d. 发射 autoEvoStep 信号
  3. 达到目标回合数 → 停止计时器 → 发射 autoEvoFinished
```

演化结束后，UI 显示各策略类型的最终数量分布和平均积分，可直观观察哪些策略在长期博弈中胜出。
