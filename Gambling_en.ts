<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="en">
<context>
    <name>Gambling</name>
    <!-- 窗口标题 -->
    <message><source>博弈游戏</source><translation>Prisoner's Dilemma</translation></message>

    <!-- 游戏页 Tab -->
    <message><source>游戏</source><translation>Game</translation></message>
    <message><source>人数设置</source><translation>Population</translation></message>
    <message><source>游戏设置</source><translation>Settings</translation></message>
    <message><source>帮助</source><translation>Help</translation></message>

    <!-- 游戏操作区 -->
    <message><source>对手：等待开始</source><translation>Opponent: Waiting</translation></message>
    <message><source>合作</source><translation>Cooperate</translation></message>
    <message><source>背叛</source><translation>Defect</translation></message>

    <!-- 自动演化区 -->
    <message><source>自动演化</source><translation>Auto Evolution</translation></message>
    <message><source>开始</source><translation>Start</translation></message>
    <message><source>暂停</source><translation>Pause</translation></message>
    <message><source>继续</source><translation>Resume</translation></message>
    <message><source>极速演化</source><translation>Fast Evo</translation></message>
    <message><source>步进</source><translation>Step</translation></message>
    <message><source>单轮</source><translation>1 Round</translation></message>

    <!-- 策略名 -->
    <message><source>诚实者</source><translation>Honest</translation></message>
    <message><source>背叛者</source><translation>Deceiver</translation></message>
    <message><source>摇摆者</source><translation>Swinger</translation></message>
    <message><source>复读者</source><translation>Repeater</translation></message>
    <message><source>宽恕者</source><translation>Forgiver</translation></message>
    <message><source>强化学习者</source><translation>Reinforcer</translation></message>
    <message><source>记仇者</source><translation>Grudger</translation></message>
    <message><source>试探者</source><translation>Detective</translation></message>
    <message><source>趋利者</source><translation>Pavlovian</translation></message>
    <message><source>从众者</source><translation>Majority</translation></message>
    <message><source>周期者</source><translation>Periodic</translation></message>

    <!-- 预设 -->
    <message><source>简单</source><translation>Easy</translation></message>
    <message><source>普通</source><translation>Normal</translation></message>
    <message><source>困难</source><translation>Hard</translation></message>
    <message><source>经典</source><translation>Classic</translation></message>

    <!-- 设置页 -->
    <message><source>&lt;b&gt;积分规则&lt;/b&gt;</source><translation>&lt;b&gt;Payoff Rules&lt;/b&gt;</translation></message>
    <message><source>双方合作</source><translation>Both Cooperate</translation></message>
    <message><source>你背叛</source><translation>You Defect</translation></message>
    <message><source>被背叛</source><translation>Betrayed</translation></message>
    <message><source>双方背叛</source><translation>Both Defect</translation></message>
    <message><source>玩家游戏轮数</source><translation>Player Rounds</translation></message>
    <message><source>继承历史</source><translation>Inherit History</translation></message>
    <message><source>启用</source><translation>Enable</translation></message>
    <message><source>淘汰（间隔/人数）</source><translation>Eliminate (Interval / Count)</translation></message>
    <message><source>失误率</source><translation>Error Rate</translation></message>
    <message><source>资产继承</source><translation>Score Carryover</translation></message>
    <message><source>随机匿名</source><translation>Hide Labels</translation></message>
    <message><source>修改NPC分数</source><translation>Modify NPC Score</translation></message>
    <message><source>设置</source><translation>Apply</translation></message>
    <message><source>总人数：</source><translation>Total: </translation></message>

    <!-- 欢迎页 -->
    <message><source>囚徒困境模拟器</source><translation>Prisoner's Dilemma Simulator</translation></message>
    <message><source>与诚实者、背叛者、复读者等多种策略的AI对手进行多轮博弈。
每轮选择合作或背叛，观察对手如何反应，
看看哪种策略能在长期博弈中胜出。</source><translation>Play repeated rounds against AI opponents with different strategies.
Choose to cooperate or defect each turn, watch how they respond,
and discover which strategies win in the long run.</translation></message>
    <message><source>开始游戏</source><translation>Start Game</translation></message>
    <message><source>中文</source><translation>中文</translation></message>

    <!-- Tooltip 翻译 -->
    <message><source>始终合作。善良但容易被剥削。</source><translation>Always cooperates. Kind but easily exploited.</translation></message>
    <message><source>始终欺骗。短期收益最高。</source><translation>Always defects. Highest short-term gain.</translation></message>
    <message><source>50% 概率随机选择，完全不可预测。</source><translation>50% random choice. Completely unpredictable.</translation></message>
    <message><source>\"以牙还牙\"。复制你上一轮的行为。</source><translation>"Tit-for-Tat." Copies your last move.</translation></message>
    <message><source>\"两错才罚\"。连续两次被欺骗才报复。</source><translation>"Tit-for-Two-Tats." Retaliates only after two consecutive defections.</translation></message>
    <message><source>ε-greedy 强化学习，根据历史选择最优应对。</source><translation>Epsilon-greedy reinforcement learning. Chooses optimal response based on history.</translation></message>
    <message><source>一旦被背叛则永不原谅该对手，始终背叛报复。</source><translation>Never forgives once betrayed. Always defects against that opponent.</translation></message>
    <message><source>前4轮试探对手：合作→背叛→合作→合作；对手背叛过则转TFT，否则一直背叛。</source><translation>First 4 turns: Cooperate→Defect→Cooperate→Cooperate. Switches to TFT if opponent defected, else always defects.</translation></message>
    <message><source>\"趋利避害\"：上一轮双方同则合作，异则背叛。</source><translation>"Win-Stay Lose-Shift." Cooperates if last round was symmetric, defects otherwise.</translation></message>
    <message><source>统计对手多数行为并跟随，平局则合作。</source><translation>Follows opponent&apos;s most frequent move. Cooperates on ties.</translation></message>
    <message><source>固定周期循环：合作→背叛→背叛→合作→重复。</source><translation>Fixed cycle: Cooperate→Defect→Defect→Cooperate, repeating.</translation></message>
    <message><source>双方合作各得奖励分。对方背叛你得被背叛分。</source><translation>Both cooperate: both get reward. Opponent defects: you get sucker&apos;s payoff.</translation></message>
    <message><source>你背叛对方：对方合作你得背叛分，对方也背叛得双方背叛分。</source><translation>You defect: opponent cooperates → you get temptation. Both defect → both get punishment.</translation></message>
    <message><source>玩家游戏的总回合数。自动演化回合数在游戏页设置。</source><translation>Total rounds in player game. Evolution rounds set on game page.</translation></message>
    <message><source>每 N 回合淘汰最低分 NPC，再由高分 NPC 克隆替代。</source><translation>Every N rounds, eliminate lowest-scoring NPCs and replace with clones of the highest scorer.</translation></message>
    <message><source>每次淘汰的人数（非百分比）。</source><translation>Number of NPCs eliminated each time (not a percentage).</translation></message>
    <message><source>克隆出的新 NPC 是否继承原体的交互历史记录。</source><translation>Whether cloned NPCs inherit the interaction history of the original.</translation></message>
    <message><source>每轮结束后按比例折算 NPC 积分（向下取整）。0%=每轮重新计分，100%=全部继承。</source><translation>Scale NPC scores by this ratio after each round (floor). 0% = reset each round, 100% = full carryover.</translation></message>
    <message><source>隐藏一定比例 NPC 的类型标签，增加游戏难度和趣味性。</source><translation>Hide a percentage of NPC type labels to increase difficulty.</translation></message>
    <message><source>要隐藏的类型标签百分比。0%=全部显示，100%=全部隐藏。</source><translation>Percentage of labels to hide. 0% = show all, 100% = hide all.</translation></message>
    <message><source>NPC 犯错概率。每个决策有该概率随机反转。</source><translation>NPC error probability. Each decision has this chance of being flipped.</translation></message>
    <message><source>双方合作时的奖励分（R）。</source><translation>Reward when both cooperate (R).</translation></message>
    <message><source>一方背叛、对方合作时，背叛方的得分（T）。</source><translation>Temptation payoff: defector&apos;s score when opponent cooperates (T).</translation></message>
    <message><source>一方合作、对方背叛时，合作方的得分（S）。</source><translation>Sucker&apos;s payoff: cooperator&apos;s score when opponent defects (S).</translation></message>
    <message><source>双方都背叛时的惩罚分（P）。</source><translation>Punishment when both defect (P).</translation></message>
    <message><source>选择要修改分数的 NPC。</source><translation>Select NPC to modify score.</translation></message>
    <message><source>设置该 NPC 的新分数。</source><translation>Set new score for this NPC.</translation></message>
    <message><source>将选定 NPC 的分数设为指定值。</source><translation>Set the selected NPC&apos;s score to the specified value.</translation></message>
    <message><source>自动演化的总回合数。</source><translation>Total rounds for auto evolution.</translation></message>
    <message><source>开始自动演化（连续运行）。</source><translation>Start auto evolution (continuous run).</translation></message>
    <message><source>极速演化：每0.05秒一整轮，无停顿。</source><translation>Fast mode: one full round every 0.05s, no pauses.</translation></message>
    <message><source>执行一对 NPC 交互。未开始时自动启动并暂停。</source><translation>Execute one NPC pair. Auto-starts and pauses if not already running.</translation></message>
    <message><source>执行一整轮 NPC 交互。未开始时自动启动并暂停。</source><translation>Execute one full NPC round. Auto-starts and pauses if not already running.</translation></message>
</context>
<context>
    <name>Welcome</name>
    <message><source>博弈游戏</source><translation>Prisoner's Dilemma</translation></message>
    <message><source>囚徒困境模拟器</source><translation>Prisoner's Dilemma Simulator</translation></message>
    <message><source>与诚实者、背叛者、复读者等多种策略的AI对手进行多轮博弈。
每轮选择合作或背叛，观察对手如何反应，
看看哪种策略能在长期博弈中胜出。</source><translation>Play repeated rounds against AI opponents with different strategies.
Choose to cooperate or defect each turn, watch how they respond,
and discover which strategies win in the long run.</translation></message>
    <message><source>开始游戏</source><translation>Start Game</translation></message>
    <message><source>中文</source><translation>中文</translation></message>
    <message><source>English</source><translation>English</translation></message>
</context>
</TS>
