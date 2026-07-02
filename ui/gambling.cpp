#include "gambling.h"
#include "ui_gambling.h"
#include "ui_welcome.h"
#include <algorithm>
#include <QMap>
#include <QStackedWidget>

Gambling::Gambling(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::Gambling)
    , m_engine(new GameEngine(this))
{
    ui->setupUi(this);
    setWindowState(Qt::WindowNoState);

    // ---- QStackedWidget 在代码中构建（不在 .ui 中定义） ----
    QStackedWidget* stack = new QStackedWidget();

    // 取出 centralWidget 布局中的 tabWidget
    QVBoxLayout* cl = qobject_cast<QVBoxLayout*>(ui->centralWidget->layout());
    cl->removeWidget(ui->tabWidget);
    stack->addWidget(ui->tabWidget);          // page 1: 游戏主界面

    // 创建欢迎页
    m_welcomePage = new QWidget();
    Ui::Welcome welcomeUi;
    welcomeUi.setupUi(m_welcomePage);
    stack->insertWidget(0, m_welcomePage);     // page 0: 欢迎页

    cl->addWidget(stack);
    stack->setCurrentIndex(0);

    // 欢迎页「开始游戏」→ 切到游戏页
    connect(welcomeUi.btnStart, &QPushButton::clicked, [stack]() {
        stack->setCurrentIndex(1);
    });

    setGameButtonsEnabled(false);
    setupTooltips();
    setupSettingsConnections();

    // ---- 人数设置 SpinBox 信号 ----
    auto connectCount = [this](QSpinBox* spin) {
        connect(spin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &Gambling::updateTotalPopulation);
    };
    connectCount(ui->spinHonestCount);
    connectCount(ui->spinDeceptiveCount);
    connectCount(ui->spinSwingerCount);
    connectCount(ui->spinRepeaterCount);
    connectCount(ui->spinReinforceCount);
    connectCount(ui->spinForgivingCount);

    updateTotalPopulation();

    // ---- GameEngine 信号连接 ----
    connect(m_engine, &GameEngine::gameStarted,       this, &Gambling::onGameStarted);
    connect(m_engine, &GameEngine::gameFinished,      this, &Gambling::onGameFinished);
    connect(m_engine, &GameEngine::roundChanged,      this, &Gambling::onRoundChanged);
    connect(m_engine, &GameEngine::playerTurn,        this, &Gambling::onPlayerTurn);
    connect(m_engine, &GameEngine::turnResult,        this, &Gambling::onTurnResult);
    connect(m_engine, &GameEngine::allScoresUpdated,  this, &Gambling::onAllScoresUpdated);
    connect(m_engine, &GameEngine::autoEvoStep,       this, &Gambling::onAutoEvoStep);
    connect(m_engine, &GameEngine::autoEvoFinished,   this, &Gambling::onAutoEvoFinished);
}

Gambling::~Gambling()
{
    delete ui;
}

// ============ Tooltip 设置 ============

void Gambling::setupTooltips()
{
    ui->spinHonestCount->setToolTip("始终合作。善良但容易被剥削——\n面对背叛者会持续吃亏。");
    ui->label->setToolTip("始终合作。善良但容易被剥削——\n面对背叛者会持续吃亏。");

    ui->spinDeceptiveCount->setToolTip("始终欺骗。短期收益最高，\n但面对复读者和宽恕者会遭到长期报复。");
    ui->label_5->setToolTip("始终欺骗。短期收益最高，\n但面对复读者和宽恕者会遭到长期报复。");

    ui->spinSwingerCount->setToolTip("每轮 50% 概率随机选择合作或欺骗。\n完全不可预测。");
    ui->label_6->setToolTip("每轮 50% 概率随机选择合作或欺骗。\n完全不可预测。");

    ui->spinRepeaterCount->setToolTip("\"以牙还牙\"。复制你上一轮的行为：\n你合作它就合作，你欺骗它就报复。\nAxelrod 博弈竞赛最成功的策略之一。");
    ui->label_7->setToolTip("\"以牙还牙\"。复制你上一轮的行为：\n你合作它就合作，你欺骗它就报复。\nAxelrod 博弈竞赛最成功的策略之一。");

    ui->spinForgivingCount->setToolTip("\"两错才罚\"。只有连续两次被欺骗才会报复。\n比复读者更宽容，容忍单次意外背叛。");
    ui->label_9->setToolTip("\"两错才罚\"。只有连续两次被欺骗才会报复。\n比复读者更宽容，容忍单次意外背叛。");

    ui->spinReinforceCount->setToolTip("为每个对手分别统计合作与欺骗的平均收益，\n选择对自己最有利的行为（90% 贪婪 + 10% 随机探索）。");
    ui->label_8->setToolTip("为每个对手分别统计合作与欺骗的平均收益，\n选择对自己最有利的行为（90% 贪婪 + 10% 随机探索）。");

    ui->btnHonest->setToolTip("双方诚实各得设置分。对方欺骗你得被背叛分。");
    ui->btnCheat->setToolTip("你欺骗对方：对方诚实你得欺骗分，\n对方也欺骗得双方欺骗分。");

    // 设置页控件 tooltip
    ui->spinRounds->setToolTip("自定义游戏总回合数（默认 10 回合）。");
    ui->chkGenetics->setToolTip("每回合按积分比例选择父代，\n用它替换最低分 NPC。\n高分 NPC 的基因更可能延续。");
    ui->spinElimination->setToolTip("每隔 N 回合执行一次淘汰。0=禁用。");
    ui->spinElimPct->setToolTip("每次淘汰时剔除积分最低的百分之多少 NPC。");
    ui->sliderErrorRate->setToolTip("NPC 每次决策时有该概率忽略策略，\n改为随机选择（玩家不受影响）。\n引入随机噪声。");
    ui->btnRanking->setToolTip("查看所有 NPC 按积分排序的实时排名。");
    ui->comboCheatNpc->setToolTip("选择要修改分数的 NPC。");
    ui->spinCheatScore->setToolTip("设定该 NPC 的分数（设为具体值）。");
    ui->btnCheatPanel->setToolTip("将选定 NPC 的分数设为指定值。");

    // 游戏页演化控件 tooltip
    ui->spinAutoEvo->setToolTip("自动演化的总回合数。");
    ui->btnEvoStart->setToolTip("开始纯 NPC 自动演化模拟。");
    ui->btnEvoPause->setToolTip("暂停/继续自动演化。");
    ui->btnEvoStep->setToolTip("暂停后每点一次执行一步。");

    // 积分规则 tooltip
    ui->labelScoreHeader->setToolTip("修改囚徒困境的支付矩阵（所有值可取负）。");
    ui->spinCooperateRwd->setToolTip("双方都诚实时的得分。");
    ui->spinCheatRwd->setToolTip("你欺骗、对方诚实时你的得分。");
    ui->spinBetrayedPnl->setToolTip("你诚实、对方欺骗时你的得分。");
    ui->spinBothCheat->setToolTip("双方都欺骗时的得分。");
}

// ============ 游戏操作按钮 ============

void Gambling::on_btnStart_clicked()
{
    int honestCount      = ui->spinHonestCount->value();
    int deceptiveCount   = ui->spinDeceptiveCount->value();
    int swingerCount     = ui->spinSwingerCount->value();
    int repeaterCount    = ui->spinRepeaterCount->value();
    int forgivingCount   = ui->spinForgivingCount->value();
    int reinforcementCount = ui->spinReinforceCount->value();

    int totalCount = honestCount + deceptiveCount + swingerCount
                   + repeaterCount + forgivingCount + reinforcementCount;
    if (totalCount == 0) {
        appendLog("错误：至少需要一个 NPC 才能开始游戏。");
        return;
    }

    ui->spinHonestCount->setEnabled(false);
    ui->spinDeceptiveCount->setEnabled(false);
    ui->spinSwingerCount->setEnabled(false);
    ui->spinRepeaterCount->setEnabled(false);
    ui->spinReinforceCount->setEnabled(false);
    ui->spinForgivingCount->setEnabled(false);

    ui->tabWidget->setCurrentIndex(0);
    ui->textLog->clear();
    ui->textOpponent->clear();
    resetStats();

    m_engine->initializeNPCs(honestCount, deceptiveCount, swingerCount,
                              repeaterCount, forgivingCount, reinforcementCount);
    m_engine->setTotalRounds(m_setRounds);
    m_engine->startGame(m_setRounds);
}

void Gambling::on_btnHonest_clicked()
{
    if (m_engine->phase() != GameEngine::WAITING_FOR_PLAYER) return;
    setGameButtonsEnabled(false);
    appendLog(QString("你选择了：%1").arg(actionName(0)));
    m_engine->playerAction(0);
}

void Gambling::on_btnCheat_clicked()
{
    if (m_engine->phase() != GameEngine::WAITING_FOR_PLAYER) return;
    setGameButtonsEnabled(false);
    appendLog(QString("你选择了：%1").arg(actionName(1)));
    m_engine->playerAction(1);
}

void Gambling::on_btnReset_clicked()
{
    m_engine->stopAutoEvolution();
    m_engine->resetGame();

    ui->textLog->clear();
    ui->textOpponent->clear();

    ui->spinHonestCount->setEnabled(true);
    ui->spinDeceptiveCount->setEnabled(true);
    ui->spinSwingerCount->setEnabled(true);
    ui->spinRepeaterCount->setEnabled(true);
    ui->spinReinforceCount->setEnabled(true);
    ui->spinForgivingCount->setEnabled(true);

    setGameButtonsEnabled(false);
    updateTotalPopulation();
    resetStats();

    // 恢复演化按钮
    ui->btnEvoStart->setEnabled(true);
    ui->btnEvoPause->setEnabled(false);
    ui->btnEvoPause->setText("暂停");
    ui->btnEvoStep->setEnabled(false);

    appendLog("游戏已重置。");
}

// ============ 自动演化按钮（游戏页） ============

void Gambling::on_btnEvoStart_clicked()
{
    auto ph = m_engine->phase();
    if (ph == GameEngine::NPC_INTERACTION || ph == GameEngine::WAITING_FOR_PLAYER) {
        appendLog("提示：请等待当前游戏结束或重置后再使用自动演化。");
        return;
    }
    int hc = ui->spinHonestCount->value(), dc = ui->spinDeceptiveCount->value();
    int sc = ui->spinSwingerCount->value(), rc = ui->spinRepeaterCount->value();
    int fc = ui->spinForgivingCount->value(), ic = ui->spinReinforceCount->value();
    if (hc + dc + sc + rc + fc + ic == 0) { appendLog("错误：至少需要一个 NPC 才能演化。"); return; }
    int rs = ui->spinAutoEvo->value();

    ui->spinHonestCount->setEnabled(false);
    ui->spinDeceptiveCount->setEnabled(false);
    ui->spinSwingerCount->setEnabled(false);
    ui->spinRepeaterCount->setEnabled(false);
    ui->spinReinforceCount->setEnabled(false);
    ui->spinForgivingCount->setEnabled(false);
    setGameButtonsEnabled(false);
    ui->btnStart->setEnabled(false);
    ui->btnReset->setEnabled(true);

    ui->btnEvoStart->setEnabled(false);
    ui->btnEvoPause->setEnabled(true);
    ui->btnEvoPause->setText("暂停");
    ui->btnEvoStep->setEnabled(false);

    ui->textLog->clear();
    ui->textOpponent->clear();
    appendLog(QString("===== 自动演化开始（%1 回合）=====").arg(rs));

    m_engine->setTotalRounds(rs);
    m_engine->startAutoEvolution(rs, hc, dc, sc, rc, fc, ic);
}

void Gambling::on_btnEvoPause_clicked()
{
    if (m_engine->phase() == GameEngine::IDLE) return;
    if (ui->btnEvoPause->text() == "暂停") {
        m_engine->pauseAutoEvolution();
        ui->btnEvoPause->setText("继续");
        ui->btnEvoStep->setEnabled(true);
        appendLog("演化已暂停。");
    } else {
        m_engine->resumeAutoEvolution();
        ui->btnEvoPause->setText("暂停");
        ui->btnEvoStep->setEnabled(false);
        appendLog("演化继续。");
    }
}

void Gambling::on_btnEvoStep_clicked()
{
    if (!m_engine->phase() == GameEngine::IDLE) {
        m_engine->stepAutoEvolution();
        appendLog("单步执行完成。");
    }
}

// ============ 人数设置 ============

void Gambling::updateTotalPopulation()
{
    int total = ui->spinHonestCount->value()
              + ui->spinDeceptiveCount->value()
              + ui->spinSwingerCount->value()
              + ui->spinRepeaterCount->value()
              + ui->spinReinforceCount->value()
              + ui->spinForgivingCount->value();
    ui->lblTotalCount->setText(QString::number(total));
}

// ============ GameEngine 信号处理 ============

void Gambling::onGameStarted()
{
    appendLog(QString("游戏开始！%1 回合博弈模拟。").arg(m_setRounds));
    updateScoreDisplay();
    setGameButtonsEnabled(true);
}

void Gambling::onGameFinished(int finalScore)
{
    setGameButtonsEnabled(false);
    updateScoreDisplay();
    appendLog(buildStatsReport(finalScore));

    ui->spinHonestCount->setEnabled(true);
    ui->spinDeceptiveCount->setEnabled(true);
    ui->spinSwingerCount->setEnabled(true);
    ui->spinRepeaterCount->setEnabled(true);
    ui->spinReinforceCount->setEnabled(true);
    ui->spinForgivingCount->setEnabled(true);
}

void Gambling::onRoundChanged(int currentRound, int totalRounds)
{
    QString info = QString("===== 第 %1 / %2 回合 =====\nNPC 间博弈完成，请选择你的行动。\n")
        .arg(currentRound).arg(totalRounds);
    appendLog(info);
    updateScoreDisplay();
}

void Gambling::onPlayerTurn(int opponentId, const QString& name,
                             const QString& strategy)
{
    Q_UNUSED(opponentId);
    m_currentOpponentStrategy = strategy;
    updateOpponentDisplay(name, strategy);
    setGameButtonsEnabled(true);
    appendLog(QString("当前对手：%1（%2）").arg(name).arg(strategy));
}

void Gambling::onTurnResult(int opponentId, const QString& name,
                             int yourAction, int opponentAction,
                             int yourScoreChange, int opponentScoreChange)
{
    Q_UNUSED(opponentId);

    m_stats.totalInteractions++;
    if (yourAction == 0) m_stats.honestCount++;
    else                 m_stats.cheatCount++;
    m_stats.scoreVsStrategy[m_currentOpponentStrategy] += yourScoreChange;
    m_stats.interactionsVsStrategy[m_currentOpponentStrategy]++;

    QString log = QString("  → 你 %1, %2 %3 | 你 %4%5分, %2 %6%7分")
        .arg(actionName(yourAction))
        .arg(name)
        .arg(actionName(opponentAction))
        .arg(yourScoreChange >= 0 ? "+" : "")
        .arg(yourScoreChange)
        .arg(opponentScoreChange >= 0 ? "+" : "")
        .arg(opponentScoreChange);
    appendLog(log);
    updateScoreDisplay();
}

void Gambling::onAllScoresUpdated(const QVector<NPCScoreInfo>& rankings)
{
    Q_UNUSED(rankings);
    updateScoreDisplay();
    // 更新作弊器的 NPC 下拉列表
    ui->comboCheatNpc->clear();
    for (const auto& n : m_engine->npcs()) {
        QString lbl = QString("%1 [%2] %3分").arg(n->getName()).arg(n->getStrategyType()).arg(n->getScore());
        ui->comboCheatNpc->addItem(lbl);
    }
}

// ============ 设置页控件信号连接 ============

void Gambling::setupSettingsConnections()
{
    // 回合数
    connect(ui->spinRounds, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int v){ m_setRounds = v; });

    // 淘汰间隔
    connect(ui->spinElimination, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int v){ m_engine->setEliminationInterval(v); });

    // 淘汰比例
    connect(ui->spinElimPct, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int v){ m_engine->setEliminationPercent(v); });

    // 遗传策略
    connect(ui->chkGenetics, &QCheckBox::toggled,
            this, [this](bool checked){ m_engine->setGeneticEnabled(checked); });

    // 失误率滑动条 → 百分比值显示 + 设置引擎
    connect(ui->sliderErrorRate, &QSlider::valueChanged,
            this, [this](int v){
        ui->lblErrorRateVal->setText(QString("%1%").arg(v));
        m_engine->setErrorRate(v / 100.0);
    });

    // 积分规则
    auto applyScoreRules = [this]() {
        m_engine->setScoreRules(
            ui->spinCooperateRwd->value(),
            ui->spinCheatRwd->value(),
            ui->spinBetrayedPnl->value(),
            ui->spinBothCheat->value());
    };
    connect(ui->spinCooperateRwd, QOverload<int>::of(&QSpinBox::valueChanged), this, applyScoreRules);
    connect(ui->spinCheatRwd,     QOverload<int>::of(&QSpinBox::valueChanged), this, applyScoreRules);
    connect(ui->spinBetrayedPnl,  QOverload<int>::of(&QSpinBox::valueChanged), this, applyScoreRules);
    connect(ui->spinBothCheat,    QOverload<int>::of(&QSpinBox::valueChanged), this, applyScoreRules);
}

// ============ 设置页按钮 ============

void Gambling::on_btnRanking_clicked()
{
    if (m_engine->phase() == GameEngine::IDLE) { appendLog("提示：请先开始游戏。"); return; }
    const auto& npcs = m_engine->npcs();
    QVector<NPCScoreInfo> rankings;
    for (const auto& npc : npcs)
        rankings.append({npc->getId(), npc->getName(), npc->getStrategyType(), npc->getScore()});
    std::sort(rankings.begin(), rankings.end(), [](const NPCScoreInfo& a, const NPCScoreInfo& b) { return a.score > b.score; });
    QString t = QString("===== 经济排名（第 %1 回合）=====\n\n玩家：%2 分\n\nNPC 排名：\n")
        .arg(m_engine->currentRound() + 1).arg(m_engine->playerScore());
    for (int i = 0; i < rankings.size(); ++i)
        t += QString("%1. %2 [%3] — %4 分\n").arg(i + 1).arg(rankings[i].name).arg(rankings[i].strategyType).arg(rankings[i].score);
    appendLog(t);
    ui->tabWidget->setCurrentIndex(0);
}

void Gambling::on_btnCheatPanel_clicked()
{
    if (m_engine->phase() == GameEngine::IDLE) { appendLog("提示：请先开始游戏。"); return; }
    int idx = ui->comboCheatNpc->currentIndex();
    if (idx < 0 || idx >= static_cast<int>(m_engine->npcs().size())) { return; }
    int score = ui->spinCheatScore->value();
    QVector<QPair<int, int>> updates;
    updates.append({idx, score});
    m_engine->setNPCScores(updates);
    appendLog(QString("[作弊] %1 分数设为 %2").arg(m_engine->npcs()[idx]->getName()).arg(score));
    ui->tabWidget->setCurrentIndex(0);
}

// ============ 自动演化信号 ============

void Gambling::onAutoEvoStep(int round, int totalRounds, const QVector<NPCScoreInfo>& rankings)
{
    Q_UNUSED(rankings);
    if (round % 5 == 0 || round == totalRounds)
        appendLog(QString("演化进度：%1 / %2 回合").arg(round).arg(totalRounds));
    updateScoreDisplay();
    // 刷新作弊器 NPC 列表
    ui->comboCheatNpc->clear();
    for (const auto& n : m_engine->npcs()) {
        QString lbl = QString("%1 [%2] %3分").arg(n->getName()).arg(n->getStrategyType()).arg(n->getScore());
        ui->comboCheatNpc->addItem(lbl);
    }
}

void Gambling::onAutoEvoFinished(const QVector<NPCScoreInfo>& finalRankings)
{
    QString r = "===== 自动演化完成 =====\n\n最终 NPC 分布：\n";
    QMap<QString, int> tcnt; QMap<QString, int> tsc;
    for (const auto& i : finalRankings) { tcnt[i.strategyType]++; tsc[i.strategyType] += i.score; }
    for (auto it = tcnt.begin(); it != tcnt.end(); ++it) {
        double a = it.value() > 0 ? static_cast<double>(tsc[it.key()]) / it.value() : 0;
        r += QString("  %1：%2 人，平均 %3 分\n").arg(it.key()).arg(it.value()).arg(a, 0, 'f', 1);
    }
    r += "\n详细排名：\n";
    for (int i = 0; i < finalRankings.size(); ++i)
        r += QString("  %1. %2 [%3] — %4 分\n").arg(i + 1).arg(finalRankings[i].name).arg(finalRankings[i].strategyType).arg(finalRankings[i].score);
    appendLog(r);

    ui->spinHonestCount->setEnabled(true);
    ui->spinDeceptiveCount->setEnabled(true);
    ui->spinSwingerCount->setEnabled(true);
    ui->spinRepeaterCount->setEnabled(true);
    ui->spinReinforceCount->setEnabled(true);
    ui->spinForgivingCount->setEnabled(true);
    ui->btnStart->setEnabled(true);

    // 恢复演化按钮
    ui->btnEvoStart->setEnabled(true);
    ui->btnEvoPause->setEnabled(false);
    ui->btnEvoPause->setText("暂停");
    ui->btnEvoStep->setEnabled(false);

    // 刷新作弊器列表
    ui->comboCheatNpc->clear();
    for (const auto& n : m_engine->npcs()) {
        QString lbl = QString("%1 [%2] %3分").arg(n->getName()).arg(n->getStrategyType()).arg(n->getScore());
        ui->comboCheatNpc->addItem(lbl);
    }
}

// ============ UI 辅助 ============

void Gambling::setGameButtonsEnabled(bool enabled)
{
    ui->btnHonest->setEnabled(enabled);
    ui->btnCheat->setEnabled(enabled);
    ui->btnStart->setEnabled(!enabled);
}

void Gambling::updateScoreDisplay()
{
    QString d;
    d += QString("你的积分：%1\n").arg(m_engine->playerScore());
    d += QString("第 %1 / %2 回合\n").arg(m_engine->currentRound() + 1).arg(m_engine->totalRounds());
    d += QString("剩余 NPC：%1\n").arg(static_cast<int>(m_engine->npcs().size()));
    ui->textLog->setText(d);
}

void Gambling::updateOpponentDisplay(const QString& name, const QString& strategy)
{
    ui->textOpponent->setText(QString("本回合对手：%1\n对手策略：%2").arg(name).arg(strategy));
}

void Gambling::appendLog(const QString& text)
{
    ui->textLog->append(text);
}

QString Gambling::actionName(int action) const
{
    return action == 0 ? "诚实" : "欺骗";
}

void Gambling::resetStats()
{
    m_stats = PlayerStats{};
}

QString Gambling::buildStatsReport(int finalScore) const
{
    QString r;
    r += "===== 游戏结束 =====\n\n";
    r += QString("■ 你的统计\n");
    r += QString("  最终积分：%1\n").arg(finalScore);
    r += QString("  总交互次数：%1\n").arg(m_stats.totalInteractions);
    if (m_stats.totalInteractions > 0) {
        r += QString("  诚实：%1 / %2 (%3%)\n")
            .arg(m_stats.honestCount).arg(m_stats.totalInteractions).arg(m_stats.honestCount * 100 / m_stats.totalInteractions);
        r += QString("  欺骗：%1 / %2 (%3%)\n")
            .arg(m_stats.cheatCount).arg(m_stats.totalInteractions).arg(m_stats.cheatCount * 100 / m_stats.totalInteractions);
    }
    if (!m_stats.scoreVsStrategy.isEmpty()) {
        r += "\n■ 对各策略的战绩\n";
        for (auto it = m_stats.scoreVsStrategy.begin(); it != m_stats.scoreVsStrategy.end(); ++it) {
            int cnt = m_stats.interactionsVsStrategy.value(it.key(), 0);
            double avg = cnt > 0 ? static_cast<double>(it.value()) / cnt : 0;
            r += QString("  %1：%2 次交互，累计 %3 分，平均 %4 分/次\n").arg(it.key()).arg(cnt).arg(it.value()).arg(avg, 0, 'f', 1);
        }
    }
    r += "\n■ NPC 排名\n";
    const auto& npcs = m_engine->npcs();
    QVector<NPCScoreInfo> rankings;
    for (const auto& npc : npcs)
        rankings.append({npc->getId(), npc->getName(), npc->getStrategyType(), npc->getScore()});
    std::sort(rankings.begin(), rankings.end(), [](const NPCScoreInfo& a, const NPCScoreInfo& b) { return a.score > b.score; });
    for (int i = 0; i < qMin(10, static_cast<int>(rankings.size())); ++i)
        r += QString("  %1. %2 [%3] — %4 分\n").arg(i + 1).arg(rankings[i].name).arg(rankings[i].strategyType).arg(rankings[i].score);
    return r;
}
