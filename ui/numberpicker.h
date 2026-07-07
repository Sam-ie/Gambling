#ifndef NUMBERPICKER_H
#define NUMBERPICKER_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QTimer>

// 手机友好的数字选择器 — 大按钮加减，无键盘弹出
class NumberPicker : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged USER true)
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum)

public:
    explicit NumberPicker(QWidget* parent = nullptr);

    int value() const { return m_value; }
    int minimum() const { return m_min; }
    int maximum() const { return m_max; }

public slots:
    void setValue(int v);
    void setMinimum(int v);
    void setMaximum(int v);

signals:
    void valueChanged(int value);

private slots:
    void onMinusPressed();
    void onPlusPressed();
    void onMinusReleased();
    void onPlusReleased();
    void onRepeatTimeout();

private:
    void updateLabel();
    void startRepeat(bool minus);
    void stopRepeat();

    int m_value = 0;
    int m_min = 0;
    int m_max = 99;

    QPushButton* m_btnMinus;
    QPushButton* m_btnPlus;
    QLabel* m_label;
    QHBoxLayout* m_layout;

    QTimer* m_repeatTimer;
    bool m_repeatDirection = false; // false=minus, true=plus
    int m_repeatCount = 0;
};

#endif // NUMBERPICKER_H
