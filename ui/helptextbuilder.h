#ifndef HELPTEXTBUILDER_H
#define HELPTEXTBUILDER_H

#include <QString>

class PayoffMatrix;
class EliminationManager;

// 帮助文本生成器：从设置参数构建 HTML 帮助文本
// 从 Gambling::updateHelpText() 中抽出，职责单一
class HelpTextBuilder {
public:
    static QString build(const PayoffMatrix& payoff,
                         const EliminationManager& eliminator,
                         int currentRound, int totalRounds,
                         int errorPct, int scoreInheritPct,
                         bool inheritHistory, int evoRounds,
                         bool english = false);
};

#endif // HELPTEXTBUILDER_H
