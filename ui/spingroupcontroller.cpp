#include "spingroupcontroller.h"
#include "numberpicker.h"

SpinGroupController::SpinGroupController(QObject* parent)
    : QObject(parent)
{
}

void SpinGroupController::registerSpin(NPCFactory::NPCType type, NumberPicker* spin)
{
    int idx = static_cast<int>(type);

    // 确保 vector 容量足够
    while (m_spins.size() <= idx) {
        m_spins.append(nullptr);
        m_types.append(static_cast<NPCFactory::NPCType>(m_types.size()));
    }

    m_spins[idx] = spin;

    connect(spin, &NumberPicker::valueChanged, this, [this]() {
        emit totalChanged(totalCount());
    });
}

void SpinGroupController::setAllEnabled(bool enabled)
{
    for (auto* spin : m_spins) {
        if (spin) spin->setEnabled(enabled);
    }
}

int SpinGroupController::totalCount() const
{
    int sum = 0;
    for (auto* spin : m_spins) {
        if (spin) sum += spin->value();
    }
    return sum;
}

NPCConfig SpinGroupController::buildConfig() const
{
    NPCConfig config;
    for (int i = 0; i < m_spins.size() && i < NPCFactory::TYPE_COUNT; ++i) {
        if (m_spins[i]) {
            config.set(static_cast<NPCFactory::NPCType>(i), m_spins[i]->value());
        }
    }
    return config;
}

void SpinGroupController::applyConfig(const NPCConfig& config)
{
    // 先全部置零
    for (auto* spin : m_spins) {
        if (spin) spin->setValue(0);
    }
    // 再设置指定值
    config.forEach([this](NPCFactory::NPCType type, int count) {
        auto* spin = spinFor(type);
        if (spin) spin->setValue(count);
    });
}

NumberPicker* SpinGroupController::spinFor(NPCFactory::NPCType type) const
{
    int idx = static_cast<int>(type);
    if (idx >= 0 && idx < m_spins.size()) return m_spins[idx];
    return nullptr;
}
