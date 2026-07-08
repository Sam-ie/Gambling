#include "gambling.h"
#include "ui_gambling.h"
#include "ui_welcome.h"
#include "npccirclewidget.h"
#include "numberpicker.h"
#include "spingroupcontroller.h"
#include "presetmanager.h"
#include "helptextbuilder.h"
#include "../core/npc/npcfactory.h"

#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QScroller>
#include <QSpacerItem>
#include <QComboBox>
#include <QApplication>

Gambling::Gambling(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::Gambling)
    , m_engine(new GameEngine(this))
    , m_spinController(new SpinGroupController(this))
    , m_presetManager(new PresetManager(m_spinController, this))
{
    ui->setupUi(this);
    setWindowState(Qt::WindowNoState);

#ifdef Q_OS_ANDROID
    // Android：移除 tabWidget 的 styleSheet，避免 QSS 触发暗色渲染（黑底白字）
    ui->tabWidget->setStyleSheet(QString());
    // 各 tab 页设 autoFillBackground，用 application palette 画亮色背景
    for (int i = 0; i < ui->tabWidget->count(); ++i)
        ui->tabWidget->widget(i)->setAutoFillBackground(true);
    // tab 标签等宽：清除 stylesheet 后失去 min-width/padding 约束，
    // Android 原生样式按文字长度计算 tab 宽度导致不等长
    QTabBar* tb = ui->tabWidget->tabBar();
    tb->setExpanding(true);
    QFont tabFont = tb->font();
    tabFont.setPointSize(14);
    tb->setFont(tabFont);
    tb->setStyleSheet("QTabBar::tab { height: 32px; min-width: 80px; padding: 0px 20px; }");
#endif

    // ---- 修复 Label + NumberPicker 布局 ----
    for (auto* np : findChildren<NumberPicker*>()) {
        QLayout* lay = np->parentWidget() ? np->parentWidget()->layout() : nullptr;
        auto* hlay = qobject_cast<QHBoxLayout*>(lay);
        if (!hlay) continue;
        int idx = hlay->indexOf(np);
        if (idx < 0) continue;
        for (int i = 0; i < idx; ++i) {
            QLayoutItem* item = hlay->itemAt(i);
            if (!item) continue;
            QLabel* lbl = qobject_cast<QLabel*>(item->widget());
            if (lbl) {
                lbl->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
                lbl->setMinimumWidth(0);
            }
        }
        auto* midSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
        hlay->insertItem(idx, midSpacer);
        hlay->setAlignment(np, Qt::AlignCenter);
    }

    // ---- Android 触摸滚动 ----
    QScroller::grabGesture(ui->textPlayerHistory->viewport(), QScroller::LeftMouseButtonGesture);
    QScroller::grabGesture(ui->textHelp->viewport(), QScroller::LeftMouseButtonGesture);
    QScroller::grabGesture(ui->lblGameInfo->viewport(), QScroller::LeftMouseButtonGesture);

    // ---- QStackedWidget 欢迎/游戏页 ----
    QStackedWidget* stack = new QStackedWidget();
    QVBoxLayout* cl = qobject_cast<QVBoxLayout*>(ui->centralWidget->layout());
    cl->removeWidget(ui->tabWidget);
    stack->addWidget(ui->tabWidget);
    m_welcomePage = new QWidget();
    m_welcomeUi = new Ui::Welcome();
    m_welcomeUi->setupUi(m_welcomePage);
#ifdef Q_OS_ANDROID
    m_welcomePage->setAutoFillBackground(true);
#endif
    stack->insertWidget(0, m_welcomePage);
    cl->addWidget(stack);
    stack->setCurrentIndex(0);

    connect(m_welcomeUi->btnStart, &QPushButton::clicked, [stack]() {
        stack->setCurrentIndex(1);
    });

    // 语言切换
    auto* comboLang = m_welcomePage->findChild<QComboBox*>("comboLanguage");
    if (comboLang) {
        comboLang->setMaxVisibleItems(6);
        connect(comboLang, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &Gambling::onLanguageChanged);
    }

    // ---- NPCCircleWidget ----
    QHBoxLayout* circleLayout = new QHBoxLayout(ui->circleContainer);
    circleLayout->setContentsMargins(0, 0, 0, 0);
    m_circleWidget = new NPCCircleWidget();
    circleLayout->addWidget(m_circleWidget);

    connect(m_circleWidget, &NPCCircleWidget::startClicked,
            this, &Gambling::onStartClicked);

    // ---- SpinGroupController ----
    setupSpinController();
    m_presetManager->apply(PresetManager::EASY);  // 默认简单模式
    setGameButtonsEnabled(false);
    setupTooltips();
    setupSettingsConnections();

    // ---- 初始化引擎参数 ----
    ui->comboCheatNpc->setMaxVisibleItems(6);
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
    connect(m_engine, &GameEngine::gameStarted,     this, &Gambling::onGameStarted);
    connect(m_engine, &GameEngine::gameFinished,    this, &Gambling::onGameFinished);
    connect(m_engine, &GameEngine::roundChanged,    this, &Gambling::onRoundChanged);
    connect(m_engine, &GameEngine::playerTurn,      this, &Gambling::onPlayerTurn);
    connect(m_engine, &GameEngine::turnResult,      this, &Gambling::onTurnResult);
    connect(m_engine, &GameEngine::allScoresUpdated,this, &Gambling::onAllScoresUpdated);
    // ---- AutoEvolutionEngine 信号直连（跳过 GameEngine 转发，避免信号-信号问题） ----
    connect(&m_engine->evoEngine(), &AutoEvolutionEngine::step,
            this, &Gambling::onAutoEvoStep);
    connect(&m_engine->evoEngine(), &AutoEvolutionEngine::pairDone,
            this, [this](int a, int b) { m_circleWidget->highlightPair(a, b); });
    connect(&m_engine->evoEngine(), &AutoEvolutionEngine::finished,
            this, &Gambling::onAutoEvoFinished);
}

Gambling::~Gambling()
{
    delete m_welcomeUi;
    delete ui;
}

// ============ SpinGroupController 注册 ============

void Gambling::setupSpinController()
{
    m_spinController->registerSpin(NPCFactory::HONEST,        ui->spinHonestCount);
    m_spinController->registerSpin(NPCFactory::DECEPTIVE,     ui->spinDeceptiveCount);
    m_spinController->registerSpin(NPCFactory::SWINGER,       ui->spinSwingerCount);
    m_spinController->registerSpin(NPCFactory::REPEATER,      ui->spinRepeaterCount);
    m_spinController->registerSpin(NPCFactory::FORGIVING,     ui->spinForgivingCount);
    m_spinController->registerSpin(NPCFactory::REINFORCEMENT, ui->spinReinforceCount);
    m_spinController->registerSpin(NPCFactory::GRUDGER,       ui->spinGrudgerCount);
    m_spinController->registerSpin(NPCFactory::DETECTIVE,     ui->spinDetectiveCount);
    m_spinController->registerSpin(NPCFactory::PAVLOV,        ui->spinPavlovCount);
    m_spinController->registerSpin(NPCFactory::MAJORITY,      ui->spinMajorityCount);
    m_spinController->registerSpin(NPCFactory::PERIODIC,      ui->spinPeriodicCount);

    connect(m_spinController, &SpinGroupController::totalChanged, this, [this](int total) {
        ui->lblTotalCount->setText(QString::number(total));
    });
    m_spinController->totalChanged(m_spinController->totalCount());
}

// ============ Tooltip ============

void Gambling::setupTooltips()
{
    struct TTip { QWidget* w; QString text; };
    const QVector<TTip> tips = {
        {ui->spinHonestCount,   tr("始终合作。善良但容易被剥削。")},
        {ui->lblHonestCount,    tr("始终合作。善良但容易被剥削。")},
        {ui->spinDeceptiveCount,tr("始终欺骗。短期收益最高。")},
        {ui->lblDeceptiveCount, tr("始终欺骗。短期收益最高。")},
        {ui->spinSwingerCount,  tr("50% 概率随机选择，完全不可预测。")},
        {ui->lblSwingerCount,   tr("50% 概率随机选择，完全不可预测。")},
        {ui->spinRepeaterCount, tr("\"以牙还牙\"。复制你上一轮的行为。")},
        {ui->lblRepeaterCount,  tr("\"以牙还牙\"。复制你上一轮的行为。")},
        {ui->spinForgivingCount,tr("\"两错才罚\"。连续两次被欺骗才报复。")},
        {ui->lblForgivingCount, tr("\"两错才罚\"。连续两次被欺骗才报复。")},
        {ui->spinReinforceCount,tr("ε-greedy 强化学习，根据历史选择最优应对。")},
        {ui->lblReinforceCount, tr("ε-greedy 强化学习，根据历史选择最优应对。")},
        {ui->spinGrudgerCount,  tr("一旦被背叛则永不原谅该对手，始终背叛报复。")},
        {ui->lblGrudgerCount,   tr("一旦被背叛则永不原谅该对手，始终背叛报复。")},
        {ui->spinDetectiveCount,tr("前4轮试探对手：合作→背叛→合作→合作；对手背叛过则转TFT，否则一直背叛。")},
        {ui->lblDetectiveCount, tr("前4轮试探对手：合作→背叛→合作→合作；对手背叛过则转TFT，否则一直背叛。")},
        {ui->spinPavlovCount,   tr("\"趋利避害\"：上一轮双方同则合作，异则背叛。")},
        {ui->lblPavlovCount,    tr("\"趋利避害\"：上一轮双方同则合作，异则背叛。")},
        {ui->spinMajorityCount, tr("统计对手多数行为并跟随，平局则合作。")},
        {ui->lblMajorityCount,  tr("统计对手多数行为并跟随，平局则合作。")},
        {ui->spinPeriodicCount, tr("固定周期循环：合作→背叛→背叛→合作→重复。")},
        {ui->lblPeriodicCount,  tr("固定周期循环：合作→背叛→背叛→合作→重复。")},
        {ui->btnHonest,         tr("双方合作各得奖励分。对方背叛你得被背叛分。")},
        {ui->btnCheat,          tr("你背叛对方：对方合作你得背叛分，对方也背叛得双方背叛分。")},
        {ui->spinRounds,        tr("玩家游戏的总回合数。自动演化回合数在游戏页设置。")},
    };
    for (const auto& t : tips) t.w->setToolTip(t.text);

    const QString elimTt = tr("每 N 回合淘汰最低分 NPC，再由高分 NPC 克隆替代。");
    ui->lblElimination->setToolTip(elimTt);
    ui->spinElimination->setToolTip(elimTt);
    ui->spinElimCount->setToolTip(tr("每次淘汰的人数（非百分比）。"));

    const QString inheritTt = tr("克隆出的新 NPC 是否继承原体的交互历史记录。");
    ui->lblInheritHistory->setToolTip(inheritTt);
    ui->chkInheritHistory->setToolTip(inheritTt);

    const QString scoreTt = tr("每轮结束后按比例折算 NPC 积分（向下取整）。0%=每轮重新计分，100%=全部继承。");
    ui->lblScoreInherit->setToolTip(scoreTt);
    ui->sliderScoreInherit->setToolTip(scoreTt);
    ui->lblScoreInheritVal->setToolTip(scoreTt);

    const QString hideTt = tr("隐藏一定比例 NPC 的类型标签，增加游戏难度和趣味性。");
    ui->lblHideLabels->setToolTip(hideTt);
    ui->chkHideLabels->setToolTip(hideTt);
    ui->sliderHideLabels->setToolTip(tr("要隐藏的类型标签百分比。0%=全部显示，100%=全部隐藏。"));
    ui->lblHideLabelsPct->setToolTip(tr("要隐藏的类型标签百分比。0%=全部显示，100%=全部隐藏。"));

    const QString errorTt = tr("NPC 犯错概率。每个决策有该概率随机反转。");
    ui->lblErrorRate->setToolTip(errorTt);
    ui->sliderErrorRate->setToolTip(errorTt);
    ui->lblErrorRateVal->setToolTip(errorTt);

    const QString payoffTt[] = {
        tr("双方合作时的奖励分（R）。"), tr("一方背叛、对方合作时，背叛方的得分（T）。"),
        tr("一方合作、对方背叛时，合作方的得分（S）。"), tr("双方都背叛时的惩罚分（P）。")
    };
    ui->lblCooperateRwd->setToolTip(payoffTt[0]);
    ui->spinCooperateRwd->setToolTip(payoffTt[0]);
    ui->lblCheatRwd->setToolTip(payoffTt[1]);
    ui->spinCheatRwd->setToolTip(payoffTt[1]);
    ui->lblBetrayedPnl->setToolTip(payoffTt[2]);
    ui->spinBetrayedPnl->setToolTip(payoffTt[2]);
    ui->lblBothCheat->setToolTip(payoffTt[3]);
    ui->spinBothCheat->setToolTip(payoffTt[3]);

    ui->lblCheatNpc->setToolTip(tr("选择要修改分数的 NPC。"));
    ui->comboCheatNpc->setToolTip(tr("选择要修改分数的 NPC。"));
    ui->spinCheatScore->setToolTip(tr("设置该 NPC 的新分数。"));
    ui->btnCheatPanel->setToolTip(tr("将选定 NPC 的分数设为指定值。"));
    ui->spinAutoEvo->setToolTip(tr("自动演化的总回合数。"));
    ui->btnEvoStart->setToolTip(tr("开始自动演化（连续运行）。"));
    ui->btnEvoFast->setToolTip(tr("极速演化：每0.05秒一整轮，无停顿。"));
    ui->btnEvoStep->setToolTip(tr("执行一对 NPC 交互。未开始时自动启动并暂停。"));
    ui->btnEvoRound->setToolTip(tr("执行一整轮 NPC 交互。未开始时自动启动并暂停。"));
}

// ============ 设置页信号 ============

void Gambling::setupSettingsConnections()
{
    ui->errorRateLayout->setAlignment(ui->sliderErrorRate, Qt::AlignVCenter);
    ui->hideLabelsLayout->setAlignment(ui->sliderHideLabels, Qt::AlignVCenter);

    connect(ui->spinRounds, &NumberPicker::valueChanged,
            this, [this](int v){ m_setRounds = v; });

    connect(ui->spinElimination, &NumberPicker::valueChanged,
            this, [this](int v){ m_engine->setEliminationInterval(v); updateHelpText(); });
    connect(ui->spinElimCount, &NumberPicker::valueChanged,
            this, [this](int v){ m_engine->setEliminationCount(v); updateHelpText(); });

    connect(ui->sliderErrorRate, &QSlider::valueChanged, this, [this](int v) {
        ui->lblErrorRateVal->setText(QString("%1%").arg(v));
        m_engine->setErrorRate(v / 100.0);
        updateHelpText();
    });

    connect(ui->chkInheritHistory, &QCheckBox::toggled, this, [this](bool chk) {
        m_engine->setInheritHistory(chk);
        updateHelpText();
    });
    m_engine->setInheritHistory(ui->chkInheritHistory->isChecked());

    connect(ui->sliderScoreInherit, &QSlider::valueChanged, this, [this](int v) {
        ui->lblScoreInheritVal->setText(QString("%1%").arg(v));
        m_engine->setScoreInheritRatio(v);
        updateHelpText();
    });
    ui->scoreInheritLayout->setAlignment(ui->sliderScoreInherit, Qt::AlignVCenter);

    connect(ui->chkHideLabels, &QCheckBox::toggled, this, [this](bool chk) {
        ui->sliderHideLabels->setEnabled(chk);
        ui->lblHideLabelsPct->setEnabled(chk);
        m_circleWidget->setHideLabelsPct(chk ? ui->sliderHideLabels->value() : 0);
    });
    connect(ui->sliderHideLabels, &QSlider::valueChanged, this, [this](int v) {
        ui->lblHideLabelsPct->setText(QString("%1%").arg(v));
        if (ui->chkHideLabels->isChecked()) m_circleWidget->setHideLabelsPct(v);
    });

    auto applyScoreRules = [this]() {
        m_engine->setScoreRules(
            ui->spinCooperateRwd->value(), ui->spinCheatRwd->value(),
            ui->spinBetrayedPnl->value(), ui->spinBothCheat->value());
        updateHelpText();
    };
    connect(ui->spinCooperateRwd, &NumberPicker::valueChanged, this, applyScoreRules);
    connect(ui->spinCheatRwd,     &NumberPicker::valueChanged, this, applyScoreRules);
    connect(ui->spinBetrayedPnl,  &NumberPicker::valueChanged, this, applyScoreRules);
    connect(ui->spinBothCheat,    &NumberPicker::valueChanged, this, applyScoreRules);
}

// ============ 圆心按钮 ============

void Gambling::onStartClicked()
{
    if (m_engine->phase() != GameEngine::IDLE || m_engine->autoEvoActive()) {
        resetUI();
        return;
    }

    NPCConfig config = m_spinController->buildConfig();
    if (config.isEmpty()) return;

    m_spinController->setAllEnabled(false);
    setGameButtonsEnabled(true);
    setPlayerMode(true);
    resetStats();

    m_engine->initializeNPCs(config);
    m_engine->setTotalRounds(m_setRounds);
    m_engine->startGame(m_setRounds);
}

void Gambling::resetUI()
{
    m_engine->stopAutoEvolution();
    m_engine->resetGame();

    m_spinController->setAllEnabled(true);
    setGameButtonsEnabled(false);
    setPlayerMode(false);

    ui->lblOpponent->setVisible(true);
    ui->btnHonest->setVisible(true);
    ui->btnCheat->setVisible(true);

    m_spinController->totalChanged(m_spinController->totalCount());
    resetStats();
    m_circleWidget->setGameRunning(false);
    m_circleWidget->setNPCData(QVector<NPCScoreInfo>());
    updateOpponentDisplay("", "");
    ui->lblGameInfo->setPlainText("已重置");

    ui->btnEvoStart->setEnabled(true);
    setEvoBtnState(EVO_IDLE);
    ui->btnEvoFast->setEnabled(true);
    ui->btnEvoStep->setEnabled(true);
    ui->btnEvoRound->setEnabled(true);
}

// ============ 游戏操作 ============

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
    switch (m_evoBtnState) {
    case EVO_IDLE: {
        auto ph = m_engine->phase();
        if (ph == GameEngine::NPC_INTERACTION || ph == GameEngine::WAITING_FOR_PLAYER) return;

        NPCConfig config = m_spinController->buildConfig();
        if (config.isEmpty()) return;
        int rs = ui->spinAutoEvo->value();

        m_engine->initializeNPCs(config);
        m_engine->setTotalRounds(rs);

        m_spinController->setAllEnabled(false);
        setGameButtonsEnabled(false);

        setEvoBtnState(EVO_RUNNING);
        ui->btnEvoFast->setEnabled(false);
        ui->btnEvoStep->setEnabled(false);
        ui->btnEvoRound->setEnabled(false);

        m_circleWidget->setGameRunning(true);
        ui->lblGameInfo->setPlainText(tr("自动演化 %1 回合").arg(rs));
        updateOpponentDisplay("", "");

        m_engine->startAutoEvolution(rs, true);
        break;
    }
    case EVO_RUNNING:
        m_engine->pauseAutoEvolution();
        setEvoBtnState(EVO_PAUSED);
        ui->btnEvoStep->setEnabled(true);
        ui->btnEvoRound->setEnabled(true);
        break;
    case EVO_PAUSED:
        m_engine->resumeAutoEvolution();
        setEvoBtnState(EVO_RUNNING);
        ui->btnEvoStep->setEnabled(false);
        ui->btnEvoRound->setEnabled(false);
        break;
    }
}

void Gambling::on_btnEvoFast_clicked()
{
    auto ph = m_engine->phase();
    if (ph == GameEngine::NPC_INTERACTION || ph == GameEngine::WAITING_FOR_PLAYER) return;

    NPCConfig config = m_spinController->buildConfig();
    if (config.isEmpty()) return;
    int rs = ui->spinAutoEvo->value();

    m_engine->initializeNPCs(config);
    m_engine->setTotalRounds(rs);

    m_spinController->setAllEnabled(false);
    setGameButtonsEnabled(false);

    setEvoBtnState(EVO_RUNNING);
    ui->btnEvoFast->setEnabled(false);
    ui->btnEvoStep->setEnabled(false);
    ui->btnEvoRound->setEnabled(false);

    m_circleWidget->setGameRunning(true);
    ui->lblGameInfo->setPlainText(QString("极速演化 %1 回合").arg(rs));
    updateOpponentDisplay("", "");

    m_engine->startAutoEvolution(rs, true);
    m_engine->startAutoEvolutionFast();
}

void Gambling::on_btnEvoStep_clicked()
{
    if (!m_engine->autoEvoActive()) {
        initAutoEvolution();
        m_engine->pauseAutoEvolution();
        setEvoBtnState(EVO_PAUSED);
        ui->btnEvoStep->setEnabled(true);
        ui->btnEvoRound->setEnabled(true);
    }
    m_engine->stepAutoEvolutionPair();
}

void Gambling::on_btnEvoRound_clicked()
{
    if (!m_engine->autoEvoActive()) {
        initAutoEvolution();
        setEvoBtnState(EVO_PAUSED);
        ui->btnEvoStep->setEnabled(true);
        ui->btnEvoRound->setEnabled(true);
    }
    m_engine->stepAutoEvolution();
}

void Gambling::initAutoEvolution()
{
    NPCConfig config = m_spinController->buildConfig();
    if (config.isEmpty()) return;
    int rs = ui->spinAutoEvo->value();

    m_engine->initializeNPCs(config);
    m_engine->setTotalRounds(rs);

    m_spinController->setAllEnabled(false);
    setGameButtonsEnabled(false);

    ui->btnEvoStart->setEnabled(false);
    ui->btnEvoFast->setEnabled(false);

    m_circleWidget->setGameRunning(true);
    ui->lblGameInfo->setPlainText(QString("自动演化 %1 回合").arg(rs));
    updateOpponentDisplay("", "");

    m_engine->startAutoEvolution(rs, false);
}

// ============ 预设 ============

void Gambling::on_btnPresetEasy_clicked()    { m_presetManager->apply(PresetManager::EASY); }
void Gambling::on_btnPresetNormal_clicked()  { m_presetManager->apply(PresetManager::NORMAL); }
void Gambling::on_btnPresetHard_clicked()    { m_presetManager->apply(PresetManager::HARD); }
void Gambling::on_btnPresetClassic_clicked() { m_presetManager->apply(PresetManager::CLASSIC); }

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
    setPlayerMode(false);
    m_circleWidget->setGameRunning(false);
    updateScoreDisplay();

    ui->lblOpponent->setVisible(false);
    ui->btnHonest->setVisible(false);
    ui->btnCheat->setVisible(false);

    QString report = buildStatsReport(finalScore);
    ui->textPlayerHistory->setVisible(true);
    ui->textPlayerHistory->setHtml(report);

    m_spinController->setAllEnabled(true);
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
    updateOpponentDisplay(name, strategy, opponentId);
    m_circleWidget->setCurrentOpponent(opponentId);
    m_circleWidget->highlightNPC(opponentId);
    showPlayerHistory(opponentId, name);
    setGameButtonsEnabled(true);
}

bool Gambling::isOpponentAnonymous(int npcId) const
{
    if (!ui->chkHideLabels->isChecked()) return false;
    int pct = ui->sliderHideLabels->value();
    return (static_cast<uint>(npcId * 2654435761U) % 100) < static_cast<uint>(pct);
}

void Gambling::showPlayerHistory(int opponentId, const QString& name)
{
    auto history = m_engine->getPlayerInteractionHistory(opponentId);
    bool anon = isOpponentAnonymous(opponentId);
    QString displayName = anon ? QStringLiteral("未知") : name;
    QString html = QString("<p><b>对手: %1</b></p>").arg(displayName);
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
    showPlayerHistory(opponentId, name);
}

void Gambling::onAllScoresUpdated(const QVector<NPCScoreInfo>& rankings)
{
    m_circleWidget->setNPCData(rankings);
    m_circleWidget->setRoundInfo(m_engine->currentRound(), m_engine->totalRounds());
    m_circleWidget->setPlayerScore(m_engine->playerScore());
    updateScoreDisplay();
    updateCheatCombo();
}

// ============ 自动演化信号 ============

void Gambling::onAutoEvoStep(int round, int totalRounds,
                              const QVector<NPCScoreInfo>& rankings)
{
    m_circleWidget->setNPCData(rankings);
    m_circleWidget->setRoundInfo(round, totalRounds);
    ui->lblGameInfo->setPlainText(QString("演化 %1 / %2 回合").arg(round).arg(totalRounds));
    updateCheatCombo();
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
    ui->lblGameInfo->setPlainText(r);

    m_spinController->setAllEnabled(true);
    ui->btnEvoStart->setEnabled(true);
    setEvoBtnState(EVO_IDLE);
    ui->btnEvoFast->setEnabled(true);
    ui->btnEvoStep->setEnabled(true);

    updateCheatCombo();
}

// ============ UI 辅助 ============

void Gambling::setPlayerMode(bool playerMode)
{
    ui->lblEvoTitle->setVisible(!playerMode);
    ui->spinAutoEvo->setVisible(!playerMode);
    ui->btnEvoStart->setVisible(!playerMode);
    ui->btnEvoFast->setVisible(!playerMode);
    ui->btnEvoStep->setVisible(!playerMode);
    ui->btnEvoRound->setVisible(!playerMode);
    ui->textPlayerHistory->setVisible(playerMode);
    ui->textPlayerHistory->clear();
    if (playerMode) {
        ui->lblOpponent->setVisible(true);
        ui->btnHonest->setVisible(true);
        ui->btnCheat->setVisible(true);
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
    ui->lblGameInfo->setPlainText(QString("回合：%1 / %2\n你的积分：%3")
        .arg(m_engine->currentRound() + 1)
        .arg(m_engine->totalRounds())
        .arg(m_engine->playerScore()));
}

void Gambling::updateOpponentDisplay(const QString& name, const QString& strategy,
                                      int opponentId)
{
    if (name.isEmpty() && strategy.isEmpty())
        ui->lblOpponent->setText("对手：等待开始");
    else {
        bool anon = isOpponentAnonymous(opponentId);
        QString dName = anon ? QStringLiteral("未知") : name;
        QString dStrat = anon ? QStringLiteral("未知") : strategy;
        ui->lblOpponent->setText(QString("对手：%1\n策略：%2").arg(dName).arg(dStrat));
    }
}

void Gambling::appendLog(const QString& text)
{
    QString cur = ui->lblGameInfo->toPlainText();
    if (cur.length() > 200) cur = cur.left(200);
    ui->lblGameInfo->setPlainText(cur + "\n" + text);
}

QString Gambling::actionName(int action) const
{
    return action == 0 ? "合作" : "背叛";
}

void Gambling::resetStats()
{
    m_stats = PlayerStats{};
}

void Gambling::updateCheatCombo()
{
    ui->comboCheatNpc->clear();
    for (const auto& n : m_engine->npcs()) {
        ui->comboCheatNpc->addItem(
            QString("%1 [%2]").arg(n->getName()).arg(n->getStrategyType()));
    }
}

// ============ 作弊 ============

void Gambling::on_btnCheatPanel_clicked()
{
    int idx = ui->comboCheatNpc->currentIndex();
    if (idx < 0 || idx >= static_cast<int>(m_engine->npcs().size())) return;
    QVector<QPair<int, int>> updates;
    updates.append({idx, ui->spinCheatScore->value()});
    m_engine->setNPCScores(updates);
}

// ============ 帮助页 ============

void Gambling::updateHelpText()
{
    int errorPct = ui->sliderErrorRate->value();
    int inheritPct = ui->sliderScoreInherit->value();
    bool inherit = ui->chkInheritHistory->isChecked();
    int evoRounds = ui->spinAutoEvo->value();

    QString html = HelpTextBuilder::build(
        m_engine->payoff(), m_engine->eliminator(),
        m_engine->currentRound(), m_engine->totalRounds(),
        errorPct, inheritPct, inherit, evoRounds, m_isEnglish);

    ui->textHelp->setHtml(html);
}

// ============ 统计报告 ============

QString Gambling::buildStatsReport(int finalScore) const
{
    QString r;
    r += "<h3>游戏结束</h3>";
    r += QString("<p><b>最终积分：%1</b></p>").arg(finalScore);
    r += QString("<p>交互：%1 次</p>").arg(m_stats.totalInteractions);
    if (m_stats.totalInteractions > 0) {
        r += QString("<p>合作 %1% / 背叛 %2%</p>")
            .arg(m_stats.honestCount * 100 / m_stats.totalInteractions)
            .arg(m_stats.cheatCount * 100 / m_stats.totalInteractions);
    }
    if (!m_stats.scoreVsStrategy.isEmpty()) {
        r += "<p><b>按策略战果：</b></p><ul>";
        for (auto it = m_stats.scoreVsStrategy.begin(); it != m_stats.scoreVsStrategy.end(); ++it) {
            int cnt = m_stats.interactionsVsStrategy.value(it.key(), 0);
            double avg = cnt > 0 ? static_cast<double>(it.value()) / cnt : 0;
            r += QString("<li>%1：%2 次，平均 %3 分/次</li>")
                .arg(it.key()).arg(cnt).arg(avg, 0, 'f', 1);
        }
        r += "</ul>";
    }
    return r;
}

// ============ 语言切换 ============

void Gambling::onLanguageChanged(int index)
{
    qApp->removeTranslator(&m_translator);

    if (index == 0) {
        // 中文：卸载翻译即为中文原文
        m_isEnglish = false;
    } else {
        // 英文：加载翻译文件
        if (m_translator.load(":/i18n/Gambling_en.qm")) {
            qApp->installTranslator(&m_translator);
        }
        m_isEnglish = true;
    }

    // 刷新 UI 文本
    ui->retranslateUi(this);
    if (m_welcomeUi) m_welcomeUi->retranslateUi(m_welcomePage);
    auto* comboLang = m_welcomePage->findChild<QComboBox*>("comboLanguage");
    if (comboLang) comboLang->blockSignals(true);
    m_circleWidget->setEnglish(m_isEnglish);
    m_circleWidget->retranslateUi();
    setEvoBtnState(m_evoBtnState);  // 恢复演化按钮状态（retranslateUi 会覆盖）
    setupTooltips();
    updateHelpText();
    if (comboLang) comboLang->blockSignals(false);
}

void Gambling::setEvoBtnState(EvoBtnState state)
{
    m_evoBtnState = state;
    switch (state) {
    case EVO_IDLE:    ui->btnEvoStart->setText(tr("开始")); break;
    case EVO_RUNNING: ui->btnEvoStart->setText(tr("暂停")); break;
    case EVO_PAUSED:  ui->btnEvoStart->setText(tr("继续")); break;
    }
}
