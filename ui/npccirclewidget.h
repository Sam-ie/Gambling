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
    void highlightPair(int npcIdA, int npcIdB);
    void setHideLabelsPct(int pct); // 隐藏类型标签比例 0-100
    void retranslateUi(); // 刷新按钮文本等
    void setEnglish(bool english); // 切换英文（图例等）

signals:
    void startClicked();
    void npcDoubleClicked(int npcId);  // 在游戏页双击NPC圆圈

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
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
    int m_hideLabelsPct = 0;   // 隐藏标签比例
    bool m_gameRunning = false;
    bool m_english = false;
    QString m_btnStartText = "开始游戏";
    QString m_btnResetText = "重置";

    // 布局缓存
    double m_centerX = 0, m_centerY = 0;
    double m_radius = 0;
    QRectF m_btnStartRect;

    // 策略颜色映射（按 NPCType 枚举索引，语言无关）
    static QMap<int, QColor> s_strategyColors;
    static QMap<QString, int> s_typeByName;
    static void initColors();
    static int typeIdxFromName(const QString& chineseName);
    QColor colorForType(int typeIdx) const;

    void recalcLayout();
    void drawBackground(QPainter& p);
    void drawNPCs(QPainter& p);
    void drawCenterButtons(QPainter& p);
    void drawLegend(QPainter& p);
};

#endif
