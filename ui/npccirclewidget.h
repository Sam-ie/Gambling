#ifndef NPCCIRCLEWIDGET_H
#define NPCCIRCLEWIDGET_H

#include <QWidget>
#include <QVector>
#include <QColor>
#include <QMap>
#include <memory>
#include "gameengine.h"

class NPCBase;
class NPCFactory;
struct NPCScoreInfo;

class NPCCircleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NPCCircleWidget(QWidget* parent = nullptr);
    ~NPCCircleWidget() override = default;

    // 数据更新接口
    void setNPCData(const QVector<NPCScoreInfo>& npcs);
    void setPlayerScore(int score);
    void setRoundInfo(int current, int total);
    void setCurrentOpponent(int npcId);  // -1 = 无
    void setGameRunning(bool running);
    void highlightNPC(int npcId);
    void highlightPair(int npcIdA, int npcIdB); // 高亮一对 NPC（自动演化连线）

signals:
    void startClicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    struct NPCVisual {
        int id;
        QString name;
        QString strategyType;
        int score;
        QColor color;
        double angle = 0;
        double x = 0, y = 0;
    };

    QVector<NPCVisual> m_npcs;
    int m_playerScore = 0;
    int m_currentRound = 0;
    int m_totalRounds = 0;
    int m_currentOpponentId = -1;
    int m_highlightId = -1;
    int m_pairHighlightA = -1; // 自动演化：突出显示的 NPC 对
    int m_pairHighlightB = -1;
    bool m_gameRunning = false;

    // 布局缓存
    double m_centerX = 0, m_centerY = 0;
    double m_radius = 0;
    QRectF m_btnStartRect;

    // 策略颜色映射
    static QMap<QString, QColor> s_strategyColors;
    static void initColors();
    QColor colorForStrategy(const QString& type) const;

    void recalcLayout();
    void drawBackground(QPainter& p);
    void drawNPCs(QPainter& p);
    void drawCenterButtons(QPainter& p);
    void drawLegend(QPainter& p);
};

#endif
