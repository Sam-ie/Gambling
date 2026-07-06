#include "npccirclewidget.h"
#include "npcfactory.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <cmath>
#include <algorithm>

QMap<QString, QColor> NPCCircleWidget::s_strategyColors;

QColor NPCCircleWidget::colorForStrategy(const QString& type) const
{
    if (s_strategyColors.contains(type))
        return s_strategyColors[type];
    return QColor("#888888");
}

void NPCCircleWidget::initColors()
{
    if (!s_strategyColors.isEmpty()) return;
    s_strategyColors["诚实者"]   = QColor("#E85A50");
    s_strategyColors["背叛者"]   = QColor("#4A4A4A");
    s_strategyColors["摇摆者"]   = QColor("#F5A623");
    s_strategyColors["复读者"]   = QColor("#3B8ED9");
    s_strategyColors["宽恕者"]   = QColor("#7B68EE");
    s_strategyColors["强化学习者"] = QColor("#50C878");
    s_strategyColors["记仇者"]   = QColor("#C62828");
    s_strategyColors["试探者"]   = QColor("#00897B");
    s_strategyColors["趋利者"]   = QColor("#FF6F00");
    s_strategyColors["从众者"]   = QColor("#6A1B9A");
    s_strategyColors["周期者"]   = QColor("#00838F");
}

NPCCircleWidget::NPCCircleWidget(QWidget* parent)
    : QWidget(parent)
{
    initColors();
    setMinimumSize(200, 200);
    setMouseTracking(false);
}

void NPCCircleWidget::setNPCData(const QVector<NPCScoreInfo>& npcs)
{
    m_npcs.clear();
    double angleStep = npcs.isEmpty() ? 0 : 2.0 * M_PI / npcs.size();
    for (int i = 0; i < npcs.size(); ++i) {
        NPCVisual v;
        v.id = npcs[i].id;
        v.name = npcs[i].name;
        v.strategyType = npcs[i].strategyType;
        v.score = npcs[i].score;
        // 匿名 NPC 使用统一灰色，否则按策略取色
        bool hidden = (m_hideLabelsPct > 0) &&
            ((static_cast<uint>(npcs[i].id * 2654435761U) % 100) < static_cast<uint>(m_hideLabelsPct));
        v.color = hidden ? QColor("#9E9E9E") : colorForStrategy(npcs[i].strategyType);
        v.angle = -M_PI / 2.0 + i * angleStep;  // 从顶部开始
        m_npcs.append(v);
    }
    recalcLayout();
    update();
}

void NPCCircleWidget::setPlayerScore(int score)
{
    m_playerScore = score;
    update();
}

void NPCCircleWidget::setRoundInfo(int current, int total)
{
    m_currentRound = current;
    m_totalRounds = total;
    update();
}

void NPCCircleWidget::setCurrentOpponent(int npcId)
{
    m_currentOpponentId = npcId;
    update();
}

void NPCCircleWidget::setGameRunning(bool running)
{
    m_gameRunning = running;
    update();
}

void NPCCircleWidget::setHideLabelsPct(int pct)
{
    m_hideLabelsPct = pct;
    update();
}

void NPCCircleWidget::highlightNPC(int npcId)
{
    m_highlightId = npcId;
    m_pairHighlightA = -1; m_pairHighlightB = -1; // 清除配对高亮
    update();
}

void NPCCircleWidget::highlightPair(int npcIdA, int npcIdB)
{
    m_pairHighlightA = npcIdA;
    m_pairHighlightB = npcIdB;
    m_highlightId = -1; // 清除单人高亮
    update();
}

void NPCCircleWidget::recalcLayout()
{
    double w = static_cast<double>(width());
    double h = static_cast<double>(height());

    // 为底部两行图例预留 56px
    double usableH = h - 56;
    m_centerX = w / 2.0;
    m_centerY = 20 + usableH / 2.0;

    // 半径 = 较小边的 42%，但不超过中心到边界的距离
    double maxR1 = m_centerX - 50;
    double maxR2 = m_centerY - 50;
    m_radius = std::min({w * 0.42, h * 0.35, maxR1, maxR2, 160.0});
    if (m_radius < 50) m_radius = 50;

    // 圆心按钮区域
    double bw = qMin(m_radius * 0.5, 120.0);
    double bh = qMin(m_radius * 0.28, 58.0);
    m_btnStartRect = QRectF(m_centerX - bw / 2, m_centerY - bh / 2, bw, bh);

    // 更新 NPC 位置
    for (auto& npc : m_npcs) {
        npc.x = m_centerX + m_radius * std::cos(npc.angle);
        npc.y = m_centerY + m_radius * std::sin(npc.angle);
    }
}

void NPCCircleWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    recalcLayout();
}

void NPCCircleWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    drawBackground(p);
    drawNPCs(p);
    drawCenterButtons(p);
    drawLegend(p);
}

void NPCCircleWidget::drawBackground(QPainter& p)
{
    p.fillRect(rect(), QColor("#F8F8F8"));
}

void NPCCircleWidget::drawNPCs(QPainter& p)
{
    if (m_npcs.isEmpty()) {
        p.setPen(QColor("#AAAAAA"));
        p.setFont(QFont("", 14));
        p.drawText(QRectF(0, m_centerY - 20, width(), 40),
                   Qt::AlignCenter, "暂无 NPC，请先设置人数");
        return;
    }

    const double nodeRadius = qMin(m_radius * 0.07, 14.0);
    if (nodeRadius < 6) return;  // 太小不画

    // 连线两个高亮的 NPC（自动演化对）
    if (m_pairHighlightA >= 0 && m_pairHighlightB >= 0) {
        const NPCVisual* a = nullptr, * b = nullptr;
        for (const auto& npc : m_npcs) {
            if (npc.id == m_pairHighlightA) a = &npc;
            if (npc.id == m_pairHighlightB) b = &npc;
        }
        if (a && b) {
            p.setPen(QPen(QColor("#FF6B35"), 2.5));
            p.drawLine(QPointF(a->x, a->y), QPointF(b->x, b->y));
        }
    }

    for (const auto& npc : m_npcs) {
        // 高亮当前对手或配对成员
        bool isHighlighted = (npc.id == m_currentOpponentId && m_gameRunning)
                          || (npc.id == m_highlightId)
                          || (npc.id == m_pairHighlightA)
                          || (npc.id == m_pairHighlightB);

        if (isHighlighted) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(255, 215, 0, 60));
            p.drawEllipse(QPointF(npc.x, npc.y), nodeRadius + 6, nodeRadius + 6);

            p.setPen(QPen(QColor("#FFB300"), 2.5));
            p.setBrush(npc.color);
            p.drawEllipse(QPointF(npc.x, npc.y), nodeRadius + 3, nodeRadius + 3);
        }

        // 节点圆
        p.setPen(Qt::NoPen);
        p.setBrush(npc.color);
        p.drawEllipse(QPointF(npc.x, npc.y), nodeRadius, nodeRadius);
    }

    // 玩家对战：绘制从圆心到当前对手的连线
    if (m_gameRunning && m_currentOpponentId >= 0) {
        for (auto& npc : m_npcs) {
            if (npc.id == m_currentOpponentId) {
                p.setPen(QPen(QColor("#FF6D00"), 2.0, Qt::DashLine));
                p.drawLine(QPointF(m_centerX, m_centerY), QPointF(npc.x, npc.y));
                break;
            }
        }
    }

    // 名字标签
    for (auto& npc : m_npcs) {
        double dx = npc.x - m_centerX;
        double dy = npc.y - m_centerY;
        double dist = std::sqrt(dx * dx + dy * dy);
        if (dist > 0) { dx /= dist; dy /= dist; }

        double labelX = npc.x + dx * (nodeRadius + 4);
        double labelY = npc.y + dy * (nodeRadius + 4);

        QFont f("", 8);
        p.setFont(f);
        p.setPen(QColor("#555555"));
        // 随机匿名：按比例隐藏类型标签
        bool hidden = (m_hideLabelsPct > 0) && ((static_cast<uint>(npc.id * 2654435761U) % 100) < static_cast<uint>(m_hideLabelsPct));
        QString displayName = hidden ? QStringLiteral("?") : (npc.name.left(1) + QString::number(npc.id));
        QString label = QString("%1 %2").arg(displayName).arg(npc.score);
        QRectF labelRect(labelX - 40, labelY - 9, 80, 18);
        p.drawText(labelRect, Qt::AlignCenter, label);
    }
}

void NPCCircleWidget::drawCenterButtons(QPainter& p)
{
    QFont btnFont("", 11);
    btnFont.setBold(true);

    // 开始按钮
    if (m_gameRunning) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#4A4A4A"));
    } else {
        p.setPen(QPen(QColor("#1D9E75"), 1.5));
        p.setBrush(QColor("#1D9E75"));
    }
    p.drawRoundedRect(m_btnStartRect, 8, 8);

    p.setPen(Qt::white);
    p.setFont(btnFont);
    p.drawText(m_btnStartRect, Qt::AlignCenter, m_gameRunning ? "重置" : "开始游戏");
}

void NPCCircleWidget::drawLegend(QPainter& p)
{
    QStringList strategies = {"诚实者", "背叛者", "摇摆者", "复读者", "宽恕者", "强化学习者",
                              "记仇者", "试探者", "趋利者", "从众者", "周期者", "匿名"};

    double w = static_cast<double>(width());
    int row1Count = strategies.size() / 2;  // 第一行 6 个
    int row2Count = strategies.size() - row1Count; // 第二行 6 个

    double itemW1 = w / row1Count;
    double itemW2 = w / row2Count;
    double legendY1 = height() - 52;
    double legendY2 = height() - 28;

    QFont f("", 8);
    p.setFont(f);

    // 第一行
    for (int i = 0; i < row1Count; ++i) {
        double cx = (i + 0.5) * itemW1;
        QColor c = (i == strategies.size() - 1)
            ? QColor("#9E9E9E")
            : colorForStrategy(strategies[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawEllipse(QPointF(cx, legendY1 + 10), 7, 7);

        p.setPen(QColor("#555555"));
        p.drawText(QRectF(cx - itemW1 / 2 + 14, legendY1 + 2, itemW1 - 16, 16),
                   Qt::AlignLeft | Qt::AlignVCenter, strategies[i]);
    }

    // 第二行
    for (int i = 0; i < row2Count; ++i) {
        int idx = row1Count + i;
        double cx = (i + 0.5) * itemW2;
        QColor c = (idx == strategies.size() - 1)
            ? QColor("#9E9E9E")
            : colorForStrategy(strategies[idx]);

        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawEllipse(QPointF(cx, legendY2 + 10), 7, 7);

        p.setPen(QColor("#555555"));
        p.drawText(QRectF(cx - itemW2 / 2 + 14, legendY2 + 2, itemW2 - 16, 16),
                   Qt::AlignLeft | Qt::AlignVCenter, strategies[idx]);
    }
}

void NPCCircleWidget::mousePressEvent(QMouseEvent* event)
{
    QPointF pt = event->position();
    if (m_btnStartRect.contains(pt)) {
        emit startClicked();
    }
    event->accept();
}
