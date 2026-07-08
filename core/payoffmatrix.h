#ifndef PAYOFFMATRIX_H
#define PAYOFFMATRIX_H

// 囚徒困境支付矩阵
// 标准参数：R=3(双方合作), T=5(背叛对方合作), S=0(合作被背叛), P=1(双方背叛)
// 约束：T > R > P > S 且 2R > T + S（保证合作是帕累托最优）
class PayoffMatrix {
public:
    PayoffMatrix(int cooperateReward = 3, int cheatReward = 5,
                 int betrayedPenalty = 0, int bothCheatPenalty = 1)
        : m_R(cooperateReward), m_T(cheatReward),
          m_S(betrayedPenalty), m_P(bothCheatPenalty) {}

    // 计算双方得分。action: 0=合作, 1=背叛
    void calculate(int actionA, int actionB, int& scoreA, int& scoreB) const {
        if (actionA == 0 && actionB == 0) {
            scoreA = m_R; scoreB = m_R;
        } else if (actionA == 0 && actionB == 1) {
            scoreA = m_S; scoreB = m_T;
        } else if (actionA == 1 && actionB == 0) {
            scoreA = m_T; scoreB = m_S;
        } else {
            scoreA = m_P; scoreB = m_P;
        }
    }

    // 访问器
    int cooperateReward()   const { return m_R; }
    int cheatReward()       const { return m_T; }
    int betrayedPenalty()   const { return m_S; }
    int bothCheatPenalty()  const { return m_P; }

    void set(int R, int T, int S, int P) { m_R = R; m_T = T; m_S = S; m_P = P; }

    // 验证是否满足囚徒困境约束
    bool isValidPD() const {
        return m_T > m_R && m_R > m_P && m_P > m_S && 2 * m_R > m_T + m_S;
    }

private:
    int m_R, m_T, m_S, m_P;
};

#endif // PAYOFFMATRIX_H
