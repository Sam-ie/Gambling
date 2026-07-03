# 囚徒困境博弈模拟器 — 设计文档

## 目录

1. [项目架构](#1-项目架构)
2. [目录结构](#2-目录结构)
3. [NPC 策略详解](#3-npc-策略详解)
4. [逻辑层设计](#4-逻辑层设计)
5. [支付矩阵](#5-支付矩阵)
6. [演化机制](#6-演化机制)
7. [可视化重构规划](#7-可视化重构规划p0p2)

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
    ├── gambling.ui             # Qt Designer 界面
    └── npccirclewidget.h/.cpp  # 圆形 NPC 可视化控件（P0 新增）
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

---

## 7. 可视化重构规划（P0~P2）

### 7.0 最终布局概览

```
横板左右分栏（gameTab 内部）
┌──────────────────────────────────────────────────────────┐
│ 左侧: NPCCircleWidget        │ 右侧: ActionPanel          │
│                              │                            │
│     ◉    ◉    ◉             │  对手：诚实者-3             │
│   ◉          ◉              │  策略：诚实 | 分数：+5      │
│  ◉   开始/步进  ◉           │  ┌────────┐ ┌────────┐    │
│   ◉          ◉              │  │  诚实  │ │  欺骗  │    │
│     ◉    ◉    ◉             │  └────────┘ └────────┘    │
│                              │  回合：3/10  积分：+16      │
│  图例：[诚][背][摇]...       │  ───────── 自动演化 ──────  │
│                              │  [回合数][开始][暂停][单步] │
└─────────────────────────────┴────────────────────────────┘
  底部 tab: [游戏(当前页)] [人数设置] [游戏设置] [帮助]
```

- **设置放在另外的页面**（人数设置 / 游戏设置 tab），不在游戏页右侧
- **右侧**只放对战操作和自动演化控制
- **每位 NPC 旁边直接显示分数**，不设独立的"查看经济排名"按钮

---

### 7.1 P0 — NPCCircleWidget（自定义绘制控件）

**文件**：`ui/npccirclewidget.h` + `ui/npccirclewidget.cpp`

**功能**：
- 继承 `QWidget`，重写 `paintEvent()` 用 `QPainter` 绘制
- 以 widget 中心为圆心，NPC 数量决定半径，均匀分布在圆周上
- 每个 NPC 绘为：**策略色圆 + 名字标签 + 积分数字**
- 当前轮到与玩家对战的 NPC **高亮**（加粗边框或脉冲圆环）
- 圆心区域放置操作按钮：开始 / 步进（覆盖 `mousePressEvent` 或嵌入子 QPushButton）

**策略颜色映射**：

| 策略 | 颜色 | 色值 |
|------|------|------|
| 诚实者 | 珊瑚红 | `#E85A50` |
| 背叛者 | 深灰 | `#4A4A4A` |
| 摇摆者 | 琥珀橙 | `#F5A623` |
| 复读者 | 天蓝 | `#3B8ED9` |
| 宽恕者 | 紫罗兰 | `#7B68EE` |
| 强化学习者 | 翠绿 | `#50C878` |

**积分显示**：每个 NPC 节点的名字/分数标签跟随圆周位置偏移（向外辐射方向）。

**图例**：底部一行显示 6 种策略的颜色圆 + 策略名。

**暴露接口**：

```cpp
class NPCCircleWidget : public QWidget {
    Q_OBJECT
public:
    void setNPCs(const std::vector<std::unique_ptr<NPCBase>>* npcs);
    void setPlayerScore(int score);
    void setCurrentRound(int round, int total);
    void setCurrentOpponent(int npcIndex);  // -1 = 无
    void setPhase(GameEngine::Phase phase);
    void highlightNPC(int npcIndex);

signals:
    void startClicked();
    void stepClicked();

protected:
    void paintEvent(QPaintEvent* event) override;
};
```

---

### 7.2 P0 — gameTab 布局改造

**文件**：`ui/gambling.ui`

**改造前 gameTab**（垂直布局）：
```
textLog → textOpponent → choiceLayout → controlLayout → evoLayout
```

**改造后 gameTab**（水平布局）：
```
QHBoxLayout
  ├─ NPCCircleWidget   [stretch=3]  左侧圆环
  └─ QWidget            [stretch=2]  右侧面板
       ├─ 对手信息区域（QLabel）
       ├─ 诚实/欺骗按钮（QHBoxLayout + 2x QPushButton）
       ├─ 回合/积分信息（QLabel）
       ├─ 分隔线（QFrame）
       └─ 自动演化区域
            ├─ 标题 "自动演化"
            ├─ spinAutoEvo + btnEvoStart + btnEvoPause + btnEvoStep
            └─ (水平一排)
```

**移除**：
- `textLog`（QTextBrowser）→ 不再需要，改用右侧面板和圆环显示
- `textOpponent`（QTextBrowser）→ 右侧面板代替
- `controlLayout` 中的 `btnStart`/`btnReset` → 移到圆心中
- `evoLayout` → 移到右侧面板

**保留底部 tab**：人数设置 / 游戏设置 / 帮助

---

### 7.3 P1 — 人数设置 tab 改版（参考项目风格）

**文件**：`ui/gambling.ui`

**改造前**：每个策略一个 SpinBox（原始，无视觉区分）

**改造后**：每个策略一行，带策略色圆 + QSlider + 值标签

```
┌──────────────────────────────────┐
│  玩家成分                        │
│                                   │
│  [●红] 诚实者    ════○──── 1     │
│  [●灰] 背叛者    ═══════○─ 2     │
│  [●橙] 摇摆者    ═○═══════ 0     │
│  [●蓝] 复读者    ═══○═════ 1     │
│  [●紫] 宽恕者    ═○═══════ 0     │
│  [●绿] 强化学习   ═══○════ 1     │
│                                   │
│  总人数：5                        │
│  [重置为1:1:1:1:1:1]             │
└──────────────────────────────────┘
```

每个策略行的高度 48px，带该策略色的左边框或背景条。
Sliders 限制：每个 0~25，总和不超过 25（参考项目逻辑），超限自动减小其他滑块。

---

### 7.4 P1 — 游戏设置 tab 改版（参考项目风格）

**文件**：`ui/gambling.ui`

**改造前**：零散排列的 SpinBox + CheckBox + Slider

**改造后**：分三个区域

**区域 1 — 报酬（支付矩阵编辑器）**：
```
┌──────────────────────────────────┐
│  报酬（积分规则）                  │
│                                   │
│           对方诚实    对方欺骗     │
│  你诚实    [ 1]        [ -1]      │
│  你欺骗    [ 2]        [ 0 ]      │
│                                   │
│  每个 SpinBox 范围 -10~10         │
└──────────────────────────────────┘
```

**区域 2 — 规则（滑块控制）**：
```
┌──────────────────────────────────┐
│  规则                            │
│                                   │
│  每局轮数    [═══○════] 10       │
│  淘汰间隔    [○═══════] 0 (关闭)│
│  淘汰比例    [═══○════] 50%      │
│  遗传策略    [启用]               │
│  失误率      [○═══════] 0%       │
└──────────────────────────────────┘
```

**区域 3 — 作弊器**：
```
┌──────────────────────────────────┐
│  作弊器                          │
│                                   │
│  [选择 NPC ▼]  [分数]  [设置]   │
└──────────────────────────────────┘
```

---

### 7.5 P1 — gambling.h/cpp 连接逻辑

**连接 NPCCircleWidget**：

```cpp
// 构造函数中
connect(m_circleWidget, &NPCCircleWidget::startClicked, this, &Gambling::onStartClicked);
connect(m_circleWidget, &NPCCircleWidget::stepClicked, this, &Gambling::onStepClicked);

// 信号处理中
connect(m_engine, &GameEngine::gameStarted, this, [this](){
    m_circleWidget->setNPCs(&m_engine->npcs());
    m_circleWidget->setPhase(m_engine->phase());
});
connect(m_engine, &GameEngine::playerTurn, this, [this](int id, const QString&, const QString&){
    m_circleWidget->setCurrentOpponent(id);
    m_circleWidget->highlightNPC(id);
});
connect(m_engine, &GameEngine::allScoresUpdated, this, [this](){
    m_circleWidget->update();
});
```

**移除的旧代码**：
- `on_btnStart_clicked()` → 重构为 `onStartClicked()`，逻辑保留
- `updateScoreDisplay()` → 简化为右侧面板刷新
- `on_btnRanking_clicked()` → 删除（分数直接显示在 NPC 图上）
- `buildStatsReport()` 保留但改为日志输出
- `textLog`/`textOpponent` 相关操作全部移除

---

### 7.6 P1 — 步进按钮整合到圆心

- 移除 `btnStart` / `btnReset` 在底部的位置
- 在 `NPCCircleWidget` 的圆心处绘制按钮区域
- 鼠标点击检测：`mousePressEvent` 中判断点击位置是否在按钮区域内
- 按钮文字在游戏进行中变为"步进"（单步推进到下一个对手）

---

### 7.7 P2 — 动画与交互细节

- **NPC 高亮**：`highlightNPC()` 为重绘当前对手添加外圈粗边框
- **淘汰/遗传动画**：被替换的 NPC 颜色渐变淡出，新 NPC 淡入（QTimer 分步重绘）
- **分数变化闪烁**：积分更新时短暂显示 "+X" 浮动文字
- **连线绘制**：当前玩家对战时可绘制从中心到高亮 NPC 的连线

---

### 7.8 实施顺序

| 阶段 | 内容 | 文件 | 备注 |
|------|------|------|------|
| **1** | NPCCircleWidget 基础绘制（圆环+颜色+名字） | npccirclewidget.h/.cpp | 先跑通绘制，数据映射后接 |
| **2** | gameTab 布局改造（水平分栏+右侧面板） | gambling.ui | UI 重建 |
| **3** | 按钮/信号连接（圆心按钮+右侧操作） | gambling.h/.cpp | 视图层改造 |
| **4** | 人数设置 tab 改滑块 | gambling.ui | P0 风格统一 |
| **5** | 游戏设置 tab 改造（矩阵+滑条+作弊） | gambling.ui | P0 风格统一 |
| **6** | 删除旧控件（textLog/ranking 等） | gambling.h/.cpp | 清理代码 |
| **7** | 动画与高亮效果 | npccirclewidget.cpp | 增强体验 |

