#include "npccirclewidget.h"
#include "npcfactory.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <cmath>
#include <algorithm>

QMap<int, QColor> NPCCircleWidget::s_strategyColors;
QMap<QString, int> NPCCircleWidget::s_typeByName;

void NPCCircleWidget::initColors()
{
    if (!s_strategyColors.isEmpty()) return;

    auto add = [](NPCFactory::NPCType type, const QColor& c) {
        int idx = static_cast<int>(type);
        s_strategyColors[idx] = c;
        s_typeByName[NPCFactory::typeName(type)] = idx;
    };

    add(NPCFactory::HONEST,        QColor("#E85A50"));
    add(NPCFactory::DECEPTIVE,     QColor("#4A4A4A"));
    add(NPCFactory::SWINGER,       QColor("#F5A623"));
    add(NPCFactory::REPEATER,      QColor("#3B8ED9"));
    add(NPCFactory::FORGIVING,     QColor("#7B68EE"));
    add(NPCFactory::REINFORCEMENT, QColor("#50C878"));
    add(NPCFactory::GRUDGER,       QColor("#C62828"));
    add(NPCFactory::DETECTIVE,     QColor("#00897B"));
    add(NPCFactory::PAVLOV,        QColor("#FF6F00"));
    add(NPCFactory::MAJORITY,      QColor("#6A1B9A"));
    add(NPCFactory::PERIODIC,      QColor("#00838F"));
}

int NPCCircleWidget::typeIdxFromName(const QString& chineseName)
{
    initColors();
    return s_typeByName.value(chineseName, -1);
}

QColor NPCCircleWidget::colorForType(int typeIdx) const
{
    if (s_strategyColors.contains(typeIdx))
        return s_strategyColors[typeIdx];
    return QColor("#9E9E9E");
}

NPCCircleWidget::NPCCircleWidget(QWidget* parent)
    : QWidget(parent)
{
    initColors();
    setMinimumSize(200, 200);
}

void NPCCircleWidget::setNPCData(const QVector<NPCScoreInfo>& npcs)
{
    m_npcs.clear();
    m_pairHighlightA = -1;
    m_pairHighlightB = -1;
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
        v.color = hidden ? QColor("#9E9E9E") : colorForType(typeIdxFromName(npcs[i].strategyType));
        v.angle = -M_PI / 2.0 + i * angleStep;  // 从顶部开始
        m_npcs.append(v);
    }
    recalcLayout();
    repaint();
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
    if (!running) {
        m_currentOpponentId = -1;
        m_pairHighlightA = -1;
        m_pairHighlightB = -1;
    }
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
    double bw = qMin(m_radius * 0.6, 160.0);
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
        double textY = m_centerY + m_radius * 0.2;
        p.drawText(QRectF(0, textY, width(), 50),
                   Qt::AlignCenter,
                   m_english ? "No NPCs — configure population first"
                             : "暂无 NPC，请先设置人数");
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
        p.setPen(QColor("#333333"));
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
    p.drawText(m_btnStartRect, Qt::AlignCenter, m_gameRunning ? m_btnResetText : m_btnStartText);
}

void NPCCircleWidget::drawLegend(QPainter& p)
{
    const auto& types = NPCFactory::allTypes();
    QStringList names;
    QString anonLabel;
    if (m_english) {
        const QStringList enNames = {"Honest", "Deceiver", "Swinger", "Repeater", "Forgiver",
                                     "Reinforcer", "Grudger", "Detective", "Pavlovian",
                                     "Majority", "Periodic"};
        names = enNames;
        anonLabel = "Hidden";
    } else {
        for (auto t : types) names.append(NPCFactory::typeName(t));
        anonLabel = "匿名";
    }
    names.append(anonLabel);

    double w = static_cast<double>(width());
    int row1Count = names.size() / 2;
    int row2Count = names.size() - row1Count;

    double itemW1 = w / row1Count;
    double itemW2 = w / row2Count;
    double legendY1 = height() - 52;
    double legendY2 = height() - 28;

    QFont f("", 8);
    p.setFont(f);
    QFontMetrics fm(f);

    // 绘制一行图例：圆圈+文字作为整体居中
    auto drawLegendRow = [&](int count, int startIdx, double itemW, double legendY,
                             const QStringList& names, const auto& types) {
        for (int i = 0; i < count; ++i) {
            int idx = startIdx + i;
            QColor c = (idx >= static_cast<int>(types.size()))
                ? QColor("#9E9E9E")
                : colorForType(static_cast<int>(types[idx]));

            const double circleR = 7;
            const double gap = 3;
            int textW = fm.horizontalAdvance(names[idx]);
            double groupW = circleR * 2 + gap + textW;
            double groupLeft = (i + 0.5) * itemW - groupW / 2;

            // 圆
            p.setPen(Qt::NoPen);
            p.setBrush(c);
            p.drawEllipse(QPointF(groupLeft + circleR, legendY + 10), circleR, circleR);

            // 文字
            p.setPen(QColor("#333333"));
            double textX = groupLeft + circleR * 2 + gap;
            p.drawText(QRectF(textX, legendY + 2, textW, 16),
                       Qt::AlignLeft | Qt::AlignVCenter, names[idx]);
        }
    };

    drawLegendRow(row1Count, 0, itemW1, legendY1, names, types);
    drawLegendRow(row2Count, row1Count, itemW2, legendY2, names, types);
}

void NPCCircleWidget::mousePressEvent(QMouseEvent* event)
{
    QPointF pt = event->position();
    if (m_btnStartRect.contains(pt)) {
        emit startClicked();
    }
    event->accept();
}

void NPCCircleWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (m_npcs.isEmpty()) return;

    QPointF pt = event->position();
    const double nodeRadius = qMin(m_radius * 0.07, 14.0);
    const double hitRadius = nodeRadius + 6;  // +6 容错（与高亮光环一致）

    for (const auto& npc : m_npcs) {
        double dx = pt.x() - npc.x;
        double dy = pt.y() - npc.y;
        if (dx * dx + dy * dy <= hitRadius * hitRadius) {
            emit npcDoubleClicked(npc.id);
            return;
        }
    }
    event->ignore();
}

void NPCCircleWidget::retranslateUi()
{
    update(); // 强制重绘以刷新按钮文本
}

void NPCCircleWidget::setEnglish(bool english)
{
    m_english = english;
    m_btnStartText = english ? "Play" : "玩家对战";
    m_btnResetText = english ? "Reset" : "重置";
    update();
}
