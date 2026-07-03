#include "gambling.h"
#include "ui_gambling.h"
#include "ui_welcome.h"
#include "npccirclewidget.h"
#include <algorithm>
#include <QMap>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QScrollBar>

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
    updateHelpText();  // 帮助页动态内容

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
    m_engine->setErrorRate(ui->sliderErrorRate->value() / 100.0);
    m_engine->setScoreInheritRatio(ui->sliderScoreInherit->value());

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

    ui->btnHonest->setToolTip("双方合作各得奖励分。对方背叛你得被背叛分。");
    ui->btnCheat->setToolTip("你背叛对方：对方合作你得背叛分，对方也背叛得双方背叛分。");
    ui->spinRounds->setToolTip("玩家游戏的总回合数。自动演化回合数在游戏页设置。");

    // 设置页标签
    ui->lblElimination->setToolTip("每 N 回合淘汰最低分 NPC，再由高分 NPC 克隆替代。");
    ui->spinElimination->setToolTip(ui->lblElimination->toolTip());
    ui->spinElimCount->setToolTip("每次淘汰的人数（非百分比）。");
    ui->lblInheritHistory->setToolTip("克隆出的新 NPC 是否继承原体的交互历史记录。");
    ui->chkInheritHistory->setToolTip(ui->lblInheritHistory->toolTip());
    ui->lblScoreInherit->setToolTip("每轮结束后按比例折算 NPC 积分（向下取整）。0%=每轮重新计分，100%=全部继承。");
    ui->sliderScoreInherit->setToolTip(ui->lblScoreInherit->toolTip());
    ui->lblScoreInheritVal->setToolTip(ui->lblScoreInherit->toolTip());
    ui->lblHideLabels->setToolTip("隐藏一定比例 NPC 的类型标签，增加游戏难度和趣味性。");
    ui->chkHideLabels->setToolTip(ui->lblHideLabels->toolTip());
    ui->sliderHideLabels->setToolTip("要隐藏的类型标签百分比。0%=全部显示，100%=全部隐藏。");
    ui->lblHideLabelsPct->setToolTip(ui->sliderHideLabels->toolTip());
    ui->lblErrorRate->setToolTip("NPC 犯错概率。每个决策有该概率随机反转。");
    ui->sliderErrorRate->setToolTip(ui->lblErrorRate->toolTip());
    ui->lblErrorRateVal->setToolTip(ui->lblErrorRate->toolTip());

    // 积分规则标签
    ui->lblCooperateRwd->setToolTip("双方合作时的奖励分（R）。");
    ui->spinCooperateRwd->setToolTip(ui->lblCooperateRwd->toolTip());
    ui->lblCheatRwd->setToolTip("一方背叛、对方合作时，背叛方的得分（T）。");
    ui->spinCheatRwd->setToolTip(ui->lblCheatRwd->toolTip());
    ui->lblBetrayedPnl->setToolTip("一方合作、对方背叛时，合作方的得分（S）。");
    ui->spinBetrayedPnl->setToolTip(ui->lblBetrayedPnl->toolTip());
    ui->lblBothCheat->setToolTip("双方都背叛时的惩罚分（P）。");
    ui->spinBothCheat->setToolTip(ui->lblBothCheat->toolTip());

    ui->lblCheatNpc->setToolTip("选择要修改分数的 NPC。");
    ui->comboCheatNpc->setToolTip(ui->lblCheatNpc->toolTip());
    ui->spinCheatScore->setToolTip("设置该 NPC 的新分数。");
    ui->btnCheatPanel->setToolTip("将选定 NPC 的分数设为指定值。");
    ui->spinAutoEvo->setToolTip("自动演化的总回合数。");
    ui->btnEvoStart->setToolTip("开始自动演化（连续运行）。");
    ui->btnEvoFast->setToolTip("极速演化：每0.05秒一整轮，无停顿。");
    ui->btnEvoStep->setToolTip("执行一对 NPC 交互。未开始时自动启动并暂停。");
    ui->btnEvoRound->setToolTip("执行一整轮 NPC 交互。未开始时自动启动并暂停。");
}

// ============ 圆心按钮 ============

void Gambling::onStartClicked()
{
    // 游戏运行中 → 重置
    if (m_engine->phase() != GameEngine::IDLE || m_engine->autoEvoActive()) {
        onResetClicked();
        return;
    }

    // 闲置状态 → 启动玩家对战
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
    setGameButtonsEnabled(true);  // 启用诚实/欺骗按钮

    // 切换到玩家模式：隐藏自动演化控件，显示历史文本框
    setPlayerMode(true);

    resetStats();
    m_engine->initializeNPCs(hc, dc, sc, rc, fc, ic);
    m_engine->setTotalRounds(m_setRounds);
    m_engine->startGame(m_setRounds);
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
    setPlayerMode(false);  // 恢复自动演化控件
    updateTotalPopulation();
    resetStats();
    m_circleWidget->setGameRunning(false);
    m_circleWidget->setNPCData(QVector<NPCScoreInfo>());
    updateOpponentDisplay("", "");
    ui->lblGameInfo->setText("已重置");

    ui->btnEvoStart->setEnabled(true);
    ui->btnEvoStart->setText("开始");
    ui->btnEvoFast->setEnabled(true);
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
    // 三重状态：开始 → 暂停 → 继续
    QString text = ui->btnEvoStart->text();
    if (text == "开始") {
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

        ui->btnEvoStart->setText("暂停");
        ui->btnEvoFast->setEnabled(false);
        ui->btnEvoStep->setEnabled(false);
        ui->btnEvoRound->setEnabled(false);

        m_circleWidget->setGameRunning(true);
        ui->lblGameInfo->setText(QString("自动演化 %1 回合").arg(rs));
        updateOpponentDisplay("", "");

        m_engine->setTotalRounds(rs);
        m_engine->startAutoEvolution(rs, hc, dc, sc, rc, fc, ic);
    } else if (text == "暂停") {
        m_engine->pauseAutoEvolution();
        ui->btnEvoStart->setText("继续");
        ui->btnEvoStep->setEnabled(true);
        ui->btnEvoRound->setEnabled(true);
    } else { // 继续
        m_engine->resumeAutoEvolution();
        ui->btnEvoStart->setText("暂停");
        ui->btnEvoStep->setEnabled(false);
        ui->btnEvoRound->setEnabled(false);
    }
}

void Gambling::on_btnEvoFast_clicked()
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

    ui->btnEvoStart->setText("暂停");
    ui->btnEvoFast->setEnabled(false);
    ui->btnEvoStep->setEnabled(false);
    ui->btnEvoRound->setEnabled(false);

    m_circleWidget->setGameRunning(true);
    ui->lblGameInfo->setText(QString("极速演化 %1 回合").arg(rs));
    updateOpponentDisplay("", "");

    m_engine->setTotalRounds(rs);
    m_engine->startAutoEvolution(rs, hc, dc, sc, rc, fc, ic);
    m_engine->startAutoEvolutionFast();
}

void Gambling::on_btnEvoStep_clicked()
{
    if (!m_engine->autoEvoActive()) {
        initAutoEvolution();
        m_engine->pauseAutoEvolution();
        ui->btnEvoStart->setText("继续");
        ui->btnEvoStep->setEnabled(true);
        ui->btnEvoRound->setEnabled(true);
    }
    m_engine->stepAutoEvolutionPair();
}

void Gambling::on_btnEvoRound_clicked()
{
    if (!m_engine->autoEvoActive()) {
        initAutoEvolution();
        ui->btnEvoStart->setText("继续");
        ui->btnEvoStep->setEnabled(true);
        ui->btnEvoRound->setEnabled(true);
    }
    m_engine->stepAutoEvolution();
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
    ui->btnEvoFast->setEnabled(false);

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
    setPlayerMode(false);  // 恢复自动演化控件
    m_circleWidget->setGameRunning(false);
    updateScoreDisplay();
    QString report = buildStatsReport(finalScore);
    ui->textPlayerHistory->setHtml(report);
    ui->textPlayerHistory->moveCursor(QTextCursor::End);

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
    showPlayerHistory(opponentId, name);
    setGameButtonsEnabled(true);
}

void Gambling::showPlayerHistory(int opponentId, const QString& name)
{
    auto history = m_engine->getPlayerInteractionHistory(opponentId);
    QString html = QString("<p><b>对手: %1</b></p>").arg(name);
    if (history.isEmpty()) {
        html += "<p>暂无历史记录</p>";
    } else {
        for (int i = 0; i < history.size(); ++i) {
            auto& pair = history[i];
            int npcAct = pair.second;
            QColor bg = (npcAct == 0) ? QColor("#E8F5E9") : QColor("#FFEBEE");
            QColor fg = (npcAct == 0) ? QColor("#2E7D32") : QColor("#C62828");
            html += QString(
                "<p style='background-color:%1; padding:2px 4px; margin:1px 0;'>"
                "回合%2: 对手 <b style='color:%3;'>%4</b> | 你 %5</p>")
                .arg(bg.name()).arg(i + 1).arg(fg.name())
                .arg(actionName(npcAct)).arg(actionName(pair.first));
        }
    }
    ui->textPlayerHistory->setHtml(html);
    ui->textPlayerHistory->verticalScrollBar()->setValue(
        ui->textPlayerHistory->verticalScrollBar()->maximum());
}

void Gambling::onTurnResult(int opponentId, const QString& name,
                             int yourAction, int /*opponentAction*/,
                             int yourScoreChange, int opponentScoreChange)
{
    Q_UNUSED(opponentId);
    Q_UNUSED(opponentScoreChange);
    m_stats.totalInteractions++;
    if (yourAction == 0) m_stats.honestCount++;
    else                 m_stats.cheatCount++;
    m_stats.scoreVsStrategy[m_currentOpponentStrategy] += yourScoreChange;
    m_stats.interactionsVsStrategy[m_currentOpponentStrategy]++;

    updateScoreDisplay();
    updateOpponentDisplay(name, m_currentOpponentStrategy);

    // 刷新历史显示
    showPlayerHistory(opponentId, name);
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
    // 失误率/隐藏标签 滑块垂直居中
    ui->errorRateLayout->setAlignment(ui->sliderErrorRate, Qt::AlignVCenter);
    ui->hideLabelsLayout->setAlignment(ui->sliderHideLabels, Qt::AlignVCenter);

    connect(ui->spinRounds, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int v){ m_setRounds = v; });
    connect(ui->spinElimination, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int v){ m_engine->setEliminationInterval(v); updateHelpText(); });
    connect(ui->spinElimCount, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int v){ m_engine->setEliminationCount(v); updateHelpText(); });
    connect(ui->sliderErrorRate, &QSlider::valueChanged,
            this, [this](int v){
        ui->lblErrorRateVal->setText(QString("%1%").arg(v));
        m_engine->setErrorRate(v / 100.0); updateHelpText();
    });

    // 继承历史
    connect(ui->chkInheritHistory, &QCheckBox::toggled,
            this, [this](bool chk){ m_engine->setInheritHistory(chk); updateHelpText(); });
    m_engine->setInheritHistory(ui->chkInheritHistory->isChecked());

    // 资产继承
    connect(ui->sliderScoreInherit, &QSlider::valueChanged,
            this, [this](int v){
        ui->lblScoreInheritVal->setText(QString("%1%").arg(v));
        m_engine->setScoreInheritRatio(v); updateHelpText();
    });
    ui->scoreInheritLayout->setAlignment(ui->sliderScoreInherit, Qt::AlignVCenter);

    // 随机匿名
    connect(ui->chkHideLabels, &QCheckBox::toggled, this, [this](bool chk) {
        ui->sliderHideLabels->setEnabled(chk);
        ui->lblHideLabelsPct->setEnabled(chk);
        m_circleWidget->setHideLabelsPct(chk ? ui->sliderHideLabels->value() : 0);
    });
    connect(ui->sliderHideLabels, &QSlider::valueChanged, this, [this](int v) {
        ui->lblHideLabelsPct->setText(QString("%1%").arg(v));
        if (ui->chkHideLabels->isChecked())
            m_circleWidget->setHideLabelsPct(v);
    });

    auto applyScoreRules = [this]() {
        m_engine->setScoreRules(
            ui->spinCooperateRwd->value(),
            ui->spinCheatRwd->value(),
            ui->spinBetrayedPnl->value(),
            ui->spinBothCheat->value());
        updateHelpText();
    };
    connect(ui->spinCooperateRwd, QOverload<int>::of(&QSpinBox::valueChanged), this, applyScoreRules);
    connect(ui->spinCheatRwd,     QOverload<int>::of(&QSpinBox::valueChanged), this, applyScoreRules);
    connect(ui->spinBetrayedPnl,  QOverload<int>::of(&QSpinBox::valueChanged), this, applyScoreRules);
    connect(ui->spinBothCheat,    QOverload<int>::of(&QSpinBox::valueChanged), this, applyScoreRules);
}

// ============ 设置页按钮 ============

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
    ui->btnEvoStart->setText("开始");
    ui->btnEvoFast->setEnabled(true);
    ui->btnEvoStep->setEnabled(true);

    ui->comboCheatNpc->clear();
    for (const auto& n : m_engine->npcs()) {
        ui->comboCheatNpc->addItem(QString("%1 [%2]").arg(n->getName()).arg(n->getStrategyType()));
    }
}

// ============ UI 辅助 ============

void Gambling::setPlayerMode(bool playerMode)
{
    // 切换玩家游戏 vs 自动演化界面
    ui->lblEvoTitle->setVisible(!playerMode);
    ui->spinAutoEvo->setVisible(!playerMode);
    ui->btnEvoStart->setVisible(!playerMode);
    ui->btnEvoFast->setVisible(!playerMode);
    ui->btnEvoStep->setVisible(!playerMode);
    ui->btnEvoRound->setVisible(!playerMode);
    ui->textPlayerHistory->setVisible(playerMode);
    ui->textPlayerHistory->clear();
    if (playerMode) {
        ui->textPlayerHistory->setHtml("<p>等待游戏开始...</p>");
    }
}

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
    return action == 0 ? "合作" : "背叛";
}

void Gambling::resetStats()
{
    m_stats = PlayerStats{};
}

// ============ 帮助页动态生成 ============

void Gambling::updateHelpText()
{
    int R = ui->spinCooperateRwd->value();
    int T = ui->spinCheatRwd->value();
    int S = ui->spinBetrayedPnl->value();
    int P = ui->spinBothCheat->value();
    int elimInt = ui->spinElimination->value();
    int elimCnt = ui->spinElimCount->value();
    int inheritPct = ui->sliderScoreInherit->value();
    int errorPct = ui->sliderErrorRate->value();
    int evoRounds = ui->spinAutoEvo->value();
    bool inherit = ui->chkInheritHistory->isChecked();

    QString html;
    html += "<h3 style='margin-top:14px; margin-bottom:12px;'><span style='font-size:large; font-weight:700;'>游戏帮助</span></h3>";

    // 积分规则
    html += "<p style='margin-top:12px;'>当前积分规则（R=" + QString::number(R)
         + ", T=" + QString::number(T) + ", S=" + QString::number(S)
         + ", P=" + QString::number(P) + "）：</p>";
    html += "<ul style='margin-top:0px;'>";
    html += "<li><b>双方合作</b>：各得 <b style='color:#2E7D32;'>" + QString::number(R) + " 分</b></li>";
    html += "<li><b>一方背叛、一方合作</b>：背叛方得 <b style='color:#C62828;'>" + QString::number(T) + " 分</b>，合作方得 <b style='color:#C62828;'>" + QString::number(S) + " 分</b></li>";
    html += "<li><b>双方背叛</b>：各得 <b style='color:#C62828;'>" + QString::number(P) + " 分</b></li>";
    html += "</ul>";

    // NPC策略
    html += "<h4 style='margin-top:14px;'><span style='font-size:medium; font-weight:700;'>6种NPC策略</span></h4>";
    html += "<ul style='margin-top:0px;'>";
    html += "<li><b>诚实者</b>：始终合作，最容易被背叛者剥削</li>";
    html += "<li><b>背叛者</b>：始终欺骗，短期收益高但长期被孤立</li>";
    html += "<li><b>摇摆者</b>：完全随机，不可预测</li>";
    html += "<li><b>复读者</b>：以牙还牙，复制你上一轮的行为</li>";
    html += "<li><b>宽恕者</b>：连续两次被背叛才会报复，宽容单次错误</li>";
    html += "<li><b>强化学习者</b>：根据历史收益选择最优策略</li>";
    html += "</ul>";

    // 当前设置
    html += "<h4 style='margin-top:14px;'><span style='font-size:medium; font-weight:700;'>当前设置</span></h4>";
    html += "<ul style='margin-top:0px;'>";
    html += "<li><b>资产继承</b>：" + QString::number(inheritPct) + "%（每轮后积分按此比例折算）</li>";
    html += "<li><b>失误率</b>：" + QString::number(errorPct) + "%</li>";
    html += "<li><b>淘汰</b>：每 " + QString::number(elimInt) + " 轮淘汰 " + QString::number(elimCnt) + " 人</li>";
    html += "<li><b>继承历史</b>：" + QString(inherit ? "启用" : "禁用") + "</li>";
    html += "<li><b>自动演化</b>：默认 " + QString::number(evoRounds) + " 回合</li>";
    html += "</ul>";


    ui->textHelp->setHtml(html);
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
