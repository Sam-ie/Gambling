#ifndef GAMBLING_H
#define GAMBLING_H

#include <QMainWindow>
#include <QTranslator>
#include <QMap>
#include "gameengine.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Gambling;
class Welcome;
}
QT_END_NAMESPACE

class NPCCircleWidget;
class SpinGroupController;
class PresetManager;

struct PlayerStats {
    int honestCount = 0;
    int cheatCount = 0;
    int totalInteractions = 0;
    QMap<QString, int> scoreVsStrategy;
    QMap<QString, int> interactionsVsStrategy;
};

class Gambling : public QMainWindow
{
    Q_OBJECT

public:
    explicit Gambling(QWidget* parent = nullptr);
    ~Gambling() override;

private slots:
    // 游戏操作
    void on_btnHonest_clicked();
    void on_btnCheat_clicked();

    // 圆心按钮
    void onStartClicked();

    // 自动演化按钮
    void on_btnEvoStart_clicked();
    void on_btnEvoFast_clicked();
    void on_btnEvoStep_clicked();
    void on_btnEvoRound_clicked();

    // 预设
    void on_btnPresetEasy_clicked();
    void on_btnPresetNormal_clicked();
    void on_btnPresetHard_clicked();
    void on_btnPresetClassic_clicked();

    // 作弊
    void on_btnCheatPanel_clicked();

    // GameEngine 信号处理
    void onGameStarted();
    void onGameFinished(int finalScore);
    void onRoundChanged(int currentRound, int totalRounds);
    void onPlayerTurn(int opponentId, const QString& name,
                      const QString& strategy);
    void onTurnResult(int opponentId, const QString& name,
                      int yourAction, int opponentAction,
                      int yourScoreChange, int opponentScoreChange);
    void onAllScoresUpdated(const QVector<NPCScoreInfo>& rankings);

    // 自动演化信号
    void onAutoEvoStep(int round, int totalRounds,
                       const QVector<NPCScoreInfo>& rankings);
    void onAutoEvoFinished(const QVector<NPCScoreInfo>& finalRankings);

    // 语言切换
    void onLanguageChanged(int index);

private:
    enum EvoBtnState { EVO_IDLE, EVO_RUNNING, EVO_PAUSED };

    Ui::Gambling* ui;
    Ui::Welcome* m_welcomeUi = nullptr;
    QWidget* m_welcomePage = nullptr;
    GameEngine* m_engine;
    NPCCircleWidget* m_circleWidget = nullptr;
    SpinGroupController* m_spinController;
    PresetManager* m_presetManager;
    QTranslator m_translator;
    int m_setRounds = 10;
    PlayerStats m_stats;
    QString m_currentOpponentStrategy;
    bool m_isEnglish = false;
    EvoBtnState m_evoBtnState = EVO_IDLE;

    void setEvoBtnState(EvoBtnState state);

    // UI 辅助
    void setupTooltips();
    void setupSettingsConnections();
    void setupSpinController();
    void initAutoEvolution();
    void resetUI();
    void setPlayerMode(bool playerMode);
    void setGameButtonsEnabled(bool enabled);
    void updateHelpText();
    void showPlayerHistory(int opponentId, const QString& name);
    void updateScoreDisplay();
    void updateOpponentDisplay(const QString& name, const QString& strategy,
                                int opponentId = -1);
    bool isOpponentAnonymous(int npcId) const;
    void appendLog(const QString& text);
    QString actionName(int action) const;
    void resetStats();
    QString buildStatsReport(int finalScore) const;
    void updateCheatCombo();
};

#endif // GAMBLING_H
