#ifndef GAMBLING_H
#define GAMBLING_H

#include <QMainWindow>
#include <QMap>
#include "gameengine.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Gambling;
class Welcome;
}
QT_END_NAMESPACE

class NPCCircleWidget;

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
    // 游戏操作按钮
    void on_btnHonest_clicked();
    void on_btnCheat_clicked();

    // 圆心按钮
    void onStartClicked();
    void onResetClicked();

    // 自动演化按钮
    void on_btnEvoStart_clicked();
    void on_btnEvoFast_clicked();
    void on_btnEvoStep_clicked();
    void on_btnEvoRound_clicked();

    // 人数设置
    void updateTotalPopulation();

    // 设置页按钮
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

    // 自动演化信号处理
    void onAutoEvoStep(int round, int totalRounds,
                       const QVector<NPCScoreInfo>& rankings);
    void onAutoEvoFinished(const QVector<NPCScoreInfo>& finalRankings);

private:
    Ui::Gambling* ui;
    QWidget* m_welcomePage = nullptr;
    GameEngine* m_engine;
    NPCCircleWidget* m_circleWidget = nullptr;
    int m_setRounds = 10;
    PlayerStats m_stats;
    QString m_currentOpponentStrategy;

    // UI 辅助
    void setupTooltips();
    void setupSettingsConnections();
    void initAutoEvolution();
    void setPlayerMode(bool playerMode);
    void setGameButtonsEnabled(bool enabled);
    void updateHelpText();
    void showPlayerHistory(int opponentId, const QString& name);
    void updateScoreDisplay();
    void updateOpponentDisplay(const QString& name, const QString& strategy);
    void appendLog(const QString& text);
    QString actionName(int action) const;
    void resetStats();
    QString buildStatsReport(int finalScore) const;
};

#endif // GAMBLING_H
