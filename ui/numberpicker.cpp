#include "numberpicker.h"

static const char* kBtnStyle =
    "QPushButton { font-size: 14px; font-weight: bold; min-width: 32px; min-height: 32px; max-width: 36px; max-height: 32px;"
    "border: 1px solid #999; background: #e8e8e8; }"
    "QPushButton:pressed { background: #c0c0c0; }";

static const char* kLabelStyle =
    "QLabel { font-size: 12px; border-top: 1px solid #999; border-bottom: 1px solid #999;"
    "background: #e8e8e8; min-width: 44px; max-width: 56px; }";

NumberPicker::NumberPicker(QWidget* parent)
    : QWidget(parent)
    , m_repeatTimer(new QTimer(this))
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(3);

    m_btnMinus = new QPushButton(QStringLiteral("\u2212"), this);
    m_btnMinus->setStyleSheet(kBtnStyle);
    m_btnMinus->setFocusPolicy(Qt::NoFocus);
    m_btnMinus->setAutoRepeat(false);

    m_label = new QLabel(this);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setStyleSheet(kLabelStyle);
    m_label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    m_btnPlus = new QPushButton("+", this);
    m_btnPlus->setStyleSheet(kBtnStyle);
    m_btnPlus->setFocusPolicy(Qt::NoFocus);
    m_btnPlus->setAutoRepeat(false);

    m_layout->addWidget(m_btnMinus);
    m_layout->addWidget(m_label);
    m_layout->addWidget(m_btnPlus);

    connect(m_btnMinus, &QPushButton::pressed, this, &NumberPicker::onMinusPressed);
    connect(m_btnPlus,  &QPushButton::pressed, this, &NumberPicker::onPlusPressed);
    connect(m_btnMinus, &QPushButton::released, this, &NumberPicker::onMinusReleased);
    connect(m_btnPlus,  &QPushButton::released, this, &NumberPicker::onPlusReleased);

    m_repeatTimer->setInterval(80);
    connect(m_repeatTimer, &QTimer::timeout, this, &NumberPicker::onRepeatTimeout);

    setMinimumSize(0, 34);
    setMaximumWidth(130);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    setFocusPolicy(Qt::NoFocus);
    updateLabel();
}

void NumberPicker::setValue(int v)
{
    v = qBound(m_min, v, m_max);
    if (v != m_value) {
        m_value = v;
        updateLabel();
        emit valueChanged(m_value);
    }
}

void NumberPicker::setMinimum(int v)
{
    m_min = v;
    if (m_value < m_min) setValue(m_min);
}

void NumberPicker::setMaximum(int v)
{
    m_max = v;
    if (m_value > m_max) setValue(m_max);
}

void NumberPicker::updateLabel()
{
    m_label->setText(QString::number(m_value));
}

void NumberPicker::onMinusPressed()
{
    if (m_value > m_min) {
        setValue(m_value - 1);
        startRepeat(false);
    }
}

void NumberPicker::onPlusPressed()
{
    if (m_value < m_max) {
        setValue(m_value + 1);
        startRepeat(true);
    }
}

void NumberPicker::onMinusReleased()  { stopRepeat(); }
void NumberPicker::onPlusReleased()   { stopRepeat(); }

void NumberPicker::startRepeat(bool plus)
{
    stopRepeat();
    m_repeatDirection = plus;
    m_repeatCount = 0;
    m_repeatTimer->start(500);  // 首次延迟 500ms，避免连跳
}

void NumberPicker::stopRepeat()
{
    m_repeatTimer->stop();
}

void NumberPicker::onRepeatTimeout()
{
    m_repeatCount++;
    // 前 3 次慢速，之后加速
    int delay = (m_repeatCount < 3) ? 400 : 80;
    m_repeatTimer->start(delay);

    if (m_repeatDirection) {
        if (m_value < m_max) setValue(m_value + 1);
        else stopRepeat();
    } else {
        if (m_value > m_min) setValue(m_value - 1);
        else stopRepeat();
    }
}
