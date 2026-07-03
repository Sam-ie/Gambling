#include "gambling.h"
#include "ui_gambling.h"
#include "ui_welcome.h"
#include "npccirclewidget.h"
#include <algorithm>
#include <QMap>
#include <QStackedWidget>
#include <QVBoxLayout>

Gambling::Gambling(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::Gambling)
    , m_engine(new GameEngine(this))
{
    ui->setupUi(this);
    setWindowState(Qt::WindowNoState);

    // ---- QStackedWidget 在代码中构建 ----
    QStackedWidget* stack = new QStackedWidget();
    QVBoxLayout* cl = qobject_cast<QVBoxLayout*>(ui->centralWidget->layout());
    cl->removeWidget(ui->tabWidget);
    stack->addWidget(ui->tabWidget);
    m_welcomePage = new QWidget();
    Ui::Welcome welcomeUi;
    welcomeUi.setupUi(m_welcomePage);
    stack->insertWidget(0, m_welcomePage);
    cl->addWidget(stack);
    stack->setCurrentIndex(0);

    connect(welcomeUi.btnStart, &QPushButton::clicked, [stack]() {
        stack->setCurrentIndex(1);
    });

    // ---- NPCCircleWidget 嵌入左侧容器 ----
    QHBoxLayout* circleLayout = new QHBoxLayout(ui->circleContainer);
    circleLayout->setContentsMargins(0, 0, 0, 0);
    m_circleWidget = new NPCCircleWidget();
    circleLayout->addWidget(m_circleWidget);

    // 圆心按钮信号
    connect(m_circleWidget, &NPCCircleWidget::startClicked,
            this, &Gambling::onStartClicked);

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

    // ---- 从设置页初始化引擎参数（构造时读一次） ----
    m_engine->setScoreRules(
        ui->spinCooperateRwd->value(),
        ui->spinCheatRwd->value(),
        ui->spinBetrayedPnl->value(),
        ui->spinBothCheat->value());
    m_engine->setEliminationInterval(ui->spinElimination->value());
    m_engine->setEliminationCount(ui->spinElimCount->value());
    m_engine->setGeneticEnabled(ui->chkGenetics->isChecked());
    m_engine->setErrorRate(ui->sliderErrorRate->value() / 100.0);

    // ---- GameEngine 信号连接 ----
    connect(m_engine, &GameEngine::gameStarted,       this, &Gambling::onGameStarted);
    connect(m_engine, &GameEngine::gameFinished,      this, &Gambling::onGameFinished);
    connect(m_engine, &GameEngine::roundChanged,      this, &Gambling::onRoundChanged);
    connect(m_engine, &GameEngine::playerTurn,        this, &Gambling::onPlayerTurn);
    connect(m_engine, &GameEngine::turnResult,        this, &Gambling::onTurnResult);
    connect(m_engine, &GameEngine::allScoresUpdated,  this, &Gambling::onAllScoresUpdated);
    connect(m_engine, &GameEngine::autoEvoStep,       this, &Gambling::onAutoEvoStep);
    connect(m_engine, &GameEngine::autoEvoPairDone,   this, [this](int a, int b) {
        m_circleWidget->highlightPair(a, b);
    });
    connect(m_engine, &GameEngine::autoEvoFinished,   this, &Gambling::onAutoEvoFinished);
}

Gambling::~Gambling()
{
    delete ui;
}

// ============ Tooltip ============

void Gambling::setupTooltips()
{
    ui->spinHonestCount->setToolTip("始终合作。善良但容易被剥削。");
    ui->label->setToolTip(ui->spinHonestCount->toolTip());
    ui->spinDeceptiveCount->setToolTip("始终欺骗。短期收益最高。");
    ui->label_5->setToolTip(ui->spinDeceptiveCount->toolTip());
    ui->spinSwingerCount->setToolTip("50% 概率随机选择，完全不可预测。");
    ui->label_6->setToolTip(ui->spinSwingerCount->toolTip());
    ui->spinRepeaterCount->setToolTip("\"以牙还牙\"。复制你上一轮的行为。");
    ui->label_7->setToolTip(ui->spinRepeaterCount->toolTip());
    ui->spinForgivingCount->setToolTip("\"两错才罚\"。连续两次被欺骗才报复。");
    ui->label_9->setToolTip(ui->spinForgivingCount->toolTip());
    ui->spinReinforceCount->setToolTip("ε-greedy 强化学习，根据历史选择最优应对。");
    ui->label_8->setToolTip(ui->spinReinforceCount->toolTip());

    ui->btnHonest->setToolTip("双方诚实各得设置分。对方欺骗你得被背叛分。");
    ui->btnCheat->setToolTip("你欺骗对方：对方诚实你得欺骗分，对方也欺骗得双方欺骗分。");
    ui->spinRounds->setToolTip("游戏总回合数。");
    ui->chkGenetics->setToolTip("强化学习者 NPC 之间遗传高分者的学习经验。");
    ui->spinElimination->setToolTip("每隔 N 回合淘汰最低分 NPC。");
    ui->spinElimCount->setToolTip("每次淘汰剔除百分之多少最低分 NPC。");
    ui->sliderErrorRate->setToolTip("NPC 失误概率。");
    ui->comboCheatNpc->setToolTip("选择要修改分数的 NPC。");
    ui->btnCheatPanel->setToolTip("将选定 NPC 的分数设为指定值。");
    ui->spinAutoEvo->setToolTip("自动演化的总回合数。");
    ui->btnEvoStart->setToolTip("开始自动演化（连续运行）。");
    ui->btnEvoPause->setToolTip("暂停/继续自动演化。");
    ui->btnEvoStep->setToolTip("执行一对 NPC 交互。未开始时自动启动并暂停。");
    ui->btnEvoRound->setToolTip("执行一整轮 NPC 交互。未开始时自动启动并暂停。");
}

// ============ 圆心按钮 ============

void Gambling::onStartClicked()
{
    if (m_engine->phase() == GameEngine::IDLE) {
        int hc = ui->spinHonestCount->value(), dc = ui->spinDeceptiveCount->value();
        int sc = ui->spinSwingerCount->value(), rc = ui->spinRepeaterCount->value();
        int fc = ui->spinForgivingCount->value(), ic = ui->spinReinforceCount->value();
        if (hc + dc + sc + rc + fc + ic == 0) return;

        ui->spinHonestCount->setEnabled(false);
        ui->spinDeceptiveCount->setEnabled(false);
        ui->spinSwingerCount->setEnabled(false);
        ui->spinRepeaterCount->setEnabled(false);
        ui->spinReinforceCount->setEnabled(false);
        ui->spinForgivingCount->setEnabled(false);

        resetStats();
        m_engine->initializeNPCs(hc, dc, sc, rc, fc, ic);
        m_engine->setTotalRounds(m_setRounds);
        m_engine->startGame(m_setRounds);
    } else {
        onResetClicked();
    }
}

void Gambling::onResetClicked()
{
    m_engine->stopAutoEvolution();
    m_engine->resetGame();

    ui->spinHonestCount->setEnabled(true);
    ui->spinDeceptiveCount->setEnabled(true);
    ui->spinSwingerCount->setEnabled(true);
    ui->spinRepeaterCount->setEnabled(true);
    ui->spinReinforceCount->setEnabled(true);
    ui->spinForgivingCount->setEnabled(true);

    setGameButtonsEnabled(false);
    updateTotalPopulation();
    resetStats();
    m_circleWidget->setGameRunning(false);
    m_circleWidget->setNPCData(QVector<NPCScoreInfo>());
    updateOpponentDisplay("", "");
    ui->lblGameInfo->setText("已重置");

    ui->btnEvoStart->setEnabled(true);
    ui->btnEvoPause->setEnabled(false);
    ui->btnEvoPause->setText("暂停");
    ui->btnEvoStep->setEnabled(true);
    ui->btnEvoRound->setEnabled(true);
}

// ============ 游戏操作按钮 ============

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

// ============ 自动演化按钮 ============

void Gambling::on_btnEvoStart_clicked()
{
    auto ph = m_engine->phase();
    if (ph == GameEngine::NPC_INTERACTION || ph == GameEngine::WAITING_FOR_PLAYER) return;

    int hc = ui->spinHonestCount->value(), dc = ui->spinDeceptiveCount->value();
    int sc = ui->spinSwingerCount->value(), rc = ui->spinRepeaterCount->value();
    int fc = ui->spinForgivingCount->value(), ic = ui->spinReinforceCount->value();
    if (hc + dc + sc + rc + fc + ic == 0) return;
    int rs = ui->spinAutoEvo->value();

    ui->spinHonestCount->setEnabled(false);
    ui->spinDeceptiveCount->setEnabled(false);
    ui->spinSwingerCount->setEnabled(false);
    ui->spinRepeaterCount->setEnabled(false);
    ui->spinReinforceCount->setEnabled(false);
    ui->spinForgivingCount->setEnabled(false);
    setGameButtonsEnabled(false);

    ui->btnEvoStart->setEnabled(false);
    ui->btnEvoPause->setEnabled(true);
    ui->btnEvoPause->setText("暂停");
    ui->btnEvoStep->setEnabled(false);
    ui->btnEvoRound->setEnabled(false);

    m_circleWidget->setGameRunning(true);
    ui->lblGameInfo->setText(QString("自动演化 %1 回合").arg(rs));
    updateOpponentDisplay("", "");

    m_engine->setTotalRounds(rs);
    m_engine->startAutoEvolution(rs, hc, dc, sc, rc, fc, ic);
}

void Gambling::on_btnEvoPause_clicked()
{
    if (ui->btnEvoPause->text() == "暂停") {
        m_engine->pauseAutoEvolution();
        ui->btnEvoPause->setText("继续");
        ui->btnEvoStep->setEnabled(true);
        ui->btnEvoRound->setEnabled(true);
    } else {
        m_engine->resumeAutoEvolution();
        ui->btnEvoPause->setText("暂停");
        ui->btnEvoStep->setEnabled(false);
        ui->btnEvoRound->setEnabled(false);
    }
}

void Gambling::on_btnEvoStep_clicked()
{
    // 如果未启动，自动初始化并暂停
    if (!m_engine->autoEvoActive()) {
        initAutoEvolution();
        m_engine->pauseAutoEvolution();
        ui->btnEvoPause->setText("继续");
        ui->btnEvoStep->setEnabled(true);
        ui->btnEvoRound->setEnabled(true);
    }
    m_engine->stepAutoEvolutionPair();
}

void Gambling::on_btnEvoRound_clicked()
{
    // 如果未启动，自动初始化并暂停
    if (!m_engine->autoEvoActive()) {
        initAutoEvolution();
        m_engine->pauseAutoEvolution();
        m_engine->stepAutoEvolution(); // 直接执行一轮
        ui->btnEvoPause->setText("继续");
        ui->btnEvoStep->setEnabled(true);
        ui->btnEvoRound->setEnabled(true);
    } else {
        m_engine->stepAutoEvolution();
    }
}

// ============ 自动演化初始化 ============

void Gambling::initAutoEvolution()
{
    int hc = ui->spinHonestCount->value(), dc = ui->spinDeceptiveCount->value();
    int sc = ui->spinSwingerCount->value(), rc = ui->spinRepeaterCount->value();
    int fc = ui->spinForgivingCount->value(), ic = ui->spinReinforceCount->value();
    if (hc + dc + sc + rc + fc + ic == 0) return;
    int rs = ui->spinAutoEvo->value();

    ui->spinHonestCount->setEnabled(false);
    ui->spinDeceptiveCount->setEnabled(false);
    ui->spinSwingerCount->setEnabled(false);
    ui->spinRepeaterCount->setEnabled(false);
    ui->spinReinforceCount->setEnabled(false);
    ui->spinForgivingCount->setEnabled(false);
    setGameButtonsEnabled(false);

    ui->btnEvoStart->setEnabled(false);
    ui->btnEvoPause->setEnabled(true);
    ui->btnEvoPause->setText("暂停");

    m_circleWidget->setGameRunning(true);
    ui->lblGameInfo->setText(QString("自动演化 %1 回合").arg(rs));
    updateOpponentDisplay("", "");

    m_engine->setTotalRounds(rs);
    m_engine->startAutoEvolution(rs, hc, dc, sc, rc, fc, ic, false);
}

// ============ 人数设置 ============

void Gambling::updateTotalPopulation()
{
    int total = ui->spinHonestCount->value() + ui->spinDeceptiveCount->value()
              + ui->spinSwingerCount->value() + ui->spinRepeaterCount->value()
              + ui->spinReinforceCount->value() + ui->spinForgivingCount->value();
    ui->lblTotalCount->setText(QString::number(total));
}

// ============ GameEngine 信号处理 ============

void Gambling::onGameStarted()
{
    m_circleWidget->setGameRunning(true);
    onAllScoresUpdated(m_engine->buildRankings());
    m_circleWidget->setRoundInfo(m_engine->currentRound(), m_engine->totalRounds());
    updateScoreDisplay();
    updateOpponentDisplay("", "");
}

void Gambling::onGameFinished(int finalScore)
{
    setGameButtonsEnabled(false);
    m_circleWidget->setGameRunning(false);
    updateScoreDisplay();
    QString report = buildStatsReport(finalScore);
    ui->lblGameInfo->setText(report.left(100));

    ui->spinHonestCount->setEnabled(true);
    ui->spinDeceptiveCount->setEnabled(true);
    ui->spinSwingerCount->setEnabled(true);
    ui->spinRepeaterCount->setEnabled(true);
    ui->spinReinforceCount->setEnabled(true);
    ui->spinForgivingCount->setEnabled(true);
}

void Gambling::onRoundChanged(int currentRound, int totalRounds)
{
    m_circleWidget->setRoundInfo(currentRound, totalRounds);
    updateScoreDisplay();
}

void Gambling::onPlayerTurn(int opponentId, const QString& name,
                             const QString& strategy)
{
    Q_UNUSED(opponentId);
    m_currentOpponentStrategy = strategy;
    updateOpponentDisplay(name, strategy);
    m_circleWidget->setCurrentOpponent(opponentId);
    m_circleWidget->highlightNPC(opponentId);
    setGameButtonsEnabled(true);
}

void Gambling::onTurnResult(int opponentId, const QString& name,
                             int yourAction, int opponentAction,
                             int yourScoreChange, int opponentScoreChange)
{
    Q_UNUSED(opponentId);
    Q_UNUSED(opponentAction);
    Q_UNUSED(opponentScoreChange);
    m_stats.totalInteractions++;
    if (yourAction == 0) m_stats.honestCount++;
    else                 m_stats.cheatCount++;
    m_stats.scoreVsStrategy[m_currentOpponentStrategy] += yourScoreChange;
    m_stats.interactionsVsStrategy[m_currentOpponentStrategy]++;

    updateScoreDisplay();
    updateOpponentDisplay(name, m_currentOpponentStrategy);
}

void Gambling::onAllScoresUpdated(const QVector<NPCScoreInfo>& rankings)
{
    m_circleWidget->setNPCData(rankings);
    m_circleWidget->setRoundInfo(m_engine->currentRound(), m_engine->totalRounds());
    m_circleWidget->setPlayerScore(m_engine->playerScore());
    updateScoreDisplay();

    ui->comboCheatNpc->clear();
    for (const auto& n : m_engine->npcs()) {
        ui->comboCheatNpc->addItem(QString("%1 [%2]").arg(n->getName()).arg(n->getStrategyType()));
    }
}

// ============ 设置页控件信号 ============

void Gambling::setupSettingsConnections()
{
    connect(ui->spinRounds, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int v){ m_setRounds = v; });
    connect(ui->spinElimination, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int v){ m_engine->setEliminationInterval(v); });
    connect(ui->spinElimCount, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int v){ m_engine->setEliminationCount(v); });
    connect(ui->chkGenetics, &QCheckBox::toggled,
            this, [this](bool chk){ m_engine->setGeneticEnabled(chk); });
    connect(ui->sliderErrorRate, &QSlider::valueChanged,
            this, [this](int v){
        ui->lblErrorRateVal->setText(QString("%1%").arg(v));
        m_engine->setErrorRate(v / 100.0);
    });

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
    const auto& npcs = m_engine->npcs();
    if (npcs.empty()) return;
    QVector<NPCScoreInfo> rankings;
    for (const auto& npc : npcs)
        rankings.append({npc->getId(), npc->getName(), npc->getStrategyType(), npc->getScore()});
    std::sort(rankings.begin(), rankings.end(),
              [](const NPCScoreInfo& a, const NPCScoreInfo& b) { return a.score > b.score; });

    QString t = QString("排名（第 %1 回合）：\n玩家 %2 分").arg(m_engine->currentRound() + 1).arg(m_engine->playerScore());
    for (int i = 0; i < qMin(20, rankings.size()); ++i)
        t += QString("\n%1. %2 [%3] %4分").arg(i + 1).arg(rankings[i].name).arg(rankings[i].strategyType).arg(rankings[i].score);
    ui->lblGameInfo->setText(t);
}

void Gambling::on_btnCheatPanel_clicked()
{
    int idx = ui->comboCheatNpc->currentIndex();
    if (idx < 0 || idx >= static_cast<int>(m_engine->npcs().size())) return;
    int score = ui->spinCheatScore->value();
    QVector<QPair<int, int>> updates;
    updates.append({idx, score});
    m_engine->setNPCScores(updates);
}

// ============ 自动演化信号 ============

void Gambling::onAutoEvoStep(int round, int totalRounds, const QVector<NPCScoreInfo>& rankings)
{
    m_circleWidget->setNPCData(rankings);
    m_circleWidget->setRoundInfo(round, totalRounds);
    ui->lblGameInfo->setText(QString("演化 %1 / %2 回合").arg(round).arg(totalRounds));

    ui->comboCheatNpc->clear();
    for (const auto& n : m_engine->npcs()) {
        ui->comboCheatNpc->addItem(QString("%1 [%2]").arg(n->getName()).arg(n->getStrategyType()));
    }
}

void Gambling::onAutoEvoFinished(const QVector<NPCScoreInfo>& finalRankings)
{
    m_circleWidget->setNPCData(finalRankings);
    m_circleWidget->setGameRunning(false);

    QMap<QString, int> tcnt;
    for (const auto& i : finalRankings) tcnt[i.strategyType]++;
    QString r = "演化结束\n";
    for (auto it = tcnt.begin(); it != tcnt.end(); ++it)
        r += QString("%1：%2人\n").arg(it.key()).arg(it.value());
    ui->lblGameInfo->setText(r);

    ui->spinHonestCount->setEnabled(true);
    ui->spinDeceptiveCount->setEnabled(true);
    ui->spinSwingerCount->setEnabled(true);
    ui->spinRepeaterCount->setEnabled(true);
    ui->spinReinforceCount->setEnabled(true);
    ui->spinForgivingCount->setEnabled(true);

    ui->btnEvoStart->setEnabled(true);
    ui->btnEvoPause->setEnabled(false);
    ui->btnEvoPause->setText("暂停");
    ui->btnEvoStep->setEnabled(true);

    ui->comboCheatNpc->clear();
    for (const auto& n : m_engine->npcs()) {
        ui->comboCheatNpc->addItem(QString("%1 [%2]").arg(n->getName()).arg(n->getStrategyType()));
    }
}

// ============ UI 辅助 ============

void Gambling::setGameButtonsEnabled(bool enabled)
{
    ui->btnHonest->setEnabled(enabled);
    ui->btnCheat->setEnabled(enabled);
}

void Gambling::updateScoreDisplay()
{
    ui->lblGameInfo->setText(QString("回合：%1 / %2\n你的积分：%3")
        .arg(m_engine->currentRound() + 1)
        .arg(m_engine->totalRounds())
        .arg(m_engine->playerScore()));
}

void Gambling::updateOpponentDisplay(const QString& name, const QString& strategy)
{
    if (name.isEmpty() && strategy.isEmpty())
        ui->lblOpponent->setText("对手：等待开始");
    else
        ui->lblOpponent->setText(QString("对手：%1\n策略：%2").arg(name).arg(strategy));
}

void Gambling::appendLog(const QString& text)
{
    QString cur = ui->lblGameInfo->text();
    if (cur.length() > 200) cur = cur.left(200);
    ui->lblGameInfo->setText(cur + "\n" + text);
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
    QString r = "游戏结束\n";
    r += QString("最终积分：%1\n").arg(finalScore);
    r += QString("交互：%1 次\n").arg(m_stats.totalInteractions);
    if (m_stats.totalInteractions > 0) {
        r += QString("诚实 %1% / 欺骗 %2%\n")
            .arg(m_stats.honestCount * 100 / m_stats.totalInteractions)
            .arg(m_stats.cheatCount * 100 / m_stats.totalInteractions);
    }
    if (!m_stats.scoreVsStrategy.isEmpty()) {
        r += "\n按策略战果：\n";
        for (auto it = m_stats.scoreVsStrategy.begin(); it != m_stats.scoreVsStrategy.end(); ++it) {
            int cnt = m_stats.interactionsVsStrategy.value(it.key(), 0);
            double avg = cnt > 0 ? static_cast<double>(it.value()) / cnt : 0;
            r += QString("  %1：%2 次，平均 %3 分/次\n").arg(it.key()).arg(cnt).arg(avg, 0, 'f', 1);
        }
    }
    return r;
}
