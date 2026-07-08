#ifndef SPINGROUPCONTROLLER_H
#define SPINGROUPCONTROLLER_H

#include <QObject>
#include <QVector>
#include "../core/npc/npcfactory.h"
#include "../core/npc/npcconfig.h"

class NumberPicker;

// 统一管理11种NPC类型对应的 NumberPicker 控件组
// 消除 Gambling 中反复出现的"禁用/启用11个spin"重复代码块
class SpinGroupController : public QObject
{
    Q_OBJECT

public:
    explicit SpinGroupController(QObject* parent = nullptr);

    // 注册一个类型的 spin 控件
    void registerSpin(NPCFactory::NPCType type, NumberPicker* spin);

    // 批量操作
    void setAllEnabled(bool enabled);
    int totalCount() const;
    NPCConfig buildConfig() const;
    void applyConfig(const NPCConfig& config);

    // 按类型访问
    NumberPicker* spinFor(NPCFactory::NPCType type) const;

    // 所有注册的 spin
    const QVector<NumberPicker*>& spins() const { return m_spins; }

signals:
    void totalChanged(int total);

private:
    QVector<NumberPicker*> m_spins;      // 按 NPCType 枚举顺序排列
    QVector<NPCFactory::NPCType> m_types;
};

#endif // SPINGROUPCONTROLLER_H
