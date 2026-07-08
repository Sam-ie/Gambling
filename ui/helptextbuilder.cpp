#include "helptextbuilder.h"
#include "../core/payoffmatrix.h"
#include "../core/elimination/eliminationmanager.h"

QString HelpTextBuilder::build(const PayoffMatrix& payoff,
                                const EliminationManager& eliminator,
                                int currentRound, int totalRounds,
                                int errorPct, int scoreInheritPct,
                                bool inheritHistory, int evoRounds,
                                bool english)
{
    Q_UNUSED(currentRound);
    Q_UNUSED(totalRounds);

    int R = payoff.cooperateReward();
    int T = payoff.cheatReward();
    int S = payoff.betrayedPenalty();
    int P = payoff.bothCheatPenalty();
    int elimInt = eliminator.interval();
    int elimCnt = eliminator.count();

    QString html;

    if (english) {
        html += "<h3 style='margin-top:14px; margin-bottom:12px;'>"
                "<span style='font-size:large; font-weight:700;'>Game Help</span></h3>";

        html += "<p style='margin-top:12px;'>Current Payoff (R=" + QString::number(R)
             + ", T=" + QString::number(T) + ", S=" + QString::number(S)
             + ", P=" + QString::number(P) + "):</p>";
        html += "<ul style='margin-top:0px;'>";
        html += "<li><b>Both Cooperate</b>: Each gets <b style='color:#2E7D32;'>" + QString::number(R) + " pts</b></li>";
        html += "<li><b>One Defects</b>: Defector gets <b style='color:#2E7D32;'>" + QString::number(T) + " pts</b>, Cooperator gets <b style='color:#C62828;'>" + QString::number(S) + " pts</b></li>";
        html += "<li><b>Both Defect</b>: Each gets <b style='color:#C62828;'>" + QString::number(P) + " pts</b></li>";
        html += "</ul>";

        html += "<h4 style='margin-top:14px;'>"
                "<span style='font-size:medium; font-weight:700;'>11 NPC Strategies</span></h4>";
        html += "<ul style='margin-top:0px;'>";
        html += "<li><b>Honest</b>: Always cooperates</li>";
        html += "<li><b>Deceiver</b>: Always defects</li>";
        html += "<li><b>Swinger</b>: Random 50/50</li>";
        html += "<li><b>Repeater</b>: Tit-for-Tat</li>";
        html += "<li><b>Forgiver</b>: Tit-for-Two-Tats</li>";
        html += "<li><b>Reinforcer</b>: Epsilon-greedy RL</li>";
        html += "<li><b>Grudger</b>: Never forgives a defection</li>";
        html += "<li><b>Detective</b>: 4-turn probe then adapt</li>";
        html += "<li><b>Pavlovian</b>: Win-Stay Lose-Shift</li>";
        html += "<li><b>Majority</b>: Follows opponent&apos;s most common move</li>";
        html += "<li><b>Periodic</b>: Fixed C-D-D-C cycle</li>";
        html += "</ul>";

        html += "<h4 style='margin-top:14px;'>"
                "<span style='font-size:medium; font-weight:700;'>Current Settings</span></h4>";
        html += "<ul style='margin-top:0px;'>";
        html += "<li><b>Score Carryover</b>: " + QString::number(scoreInheritPct) + "%</li>";
        html += "<li><b>Error Rate</b>: " + QString::number(errorPct) + "%</li>";
        html += "<li><b>Elimination</b>: Every " + QString::number(elimInt) + " rounds, eliminate " + QString::number(elimCnt) + "</li>";
        html += "<li><b>Inherit History</b>: " + QString(inheritHistory ? "On" : "Off") + "</li>";
        html += "<li><b>Auto Evolution</b>: Default " + QString::number(evoRounds) + " rounds</li>";
        html += "</ul>";
    } else {
        html += "<h3 style='margin-top:14px; margin-bottom:12px;'>"
                "<span style='font-size:large; font-weight:700;'>游戏帮助</span></h3>";

        html += "<p style='margin-top:12px;'>当前积分规则（R=" + QString::number(R)
             + ", T=" + QString::number(T) + ", S=" + QString::number(S)
             + ", P=" + QString::number(P) + "）：</p>";
        html += "<ul style='margin-top:0px;'>";
        html += "<li><b>双方合作</b>：各得 <b style='color:#2E7D32;'>" + QString::number(R) + " 分</b></li>";
        html += "<li><b>一方背叛、一方合作</b>：背叛方得 <b style='color:#2E7D32;'>" + QString::number(T) + " 分</b>，合作方得 <b style='color:#C62828;'>" + QString::number(S) + " 分</b></li>";
        html += "<li><b>双方背叛</b>：各得 <b style='color:#C62828;'>" + QString::number(P) + " 分</b></li>";
        html += "</ul>";

        html += "<h4 style='margin-top:14px;'>"
                "<span style='font-size:medium; font-weight:700;'>11种NPC策略</span></h4>";
        html += "<ul style='margin-top:0px;'>";
        html += "<li><b>诚实者</b>：始终合作，最容易被背叛者剥削</li>";
        html += "<li><b>背叛者</b>：始终欺骗，短期收益高但长期被孤立</li>";
        html += "<li><b>摇摆者</b>：完全随机，不可预测</li>";
        html += "<li><b>复读者</b>：以牙还牙，复制你上一轮的行为</li>";
        html += "<li><b>宽恕者</b>：连续两次被背叛才会报复，宽容单次错误</li>";
        html += "<li><b>强化学习者</b>：根据历史收益选择最优策略</li>";
        html += "<li><b>记仇者</b>：一旦被背叛则永不原谅该对手</li>";
        html += "<li><b>试探者</b>：前4轮试探，对手有背叛则转TFT，否则一直背叛</li>";
        html += "<li><b>趋利者</b>：上一轮双方同则合作，异则背叛</li>";
        html += "<li><b>从众者</b>：统计对手历史，跟随多数行为</li>";
        html += "<li><b>周期者</b>：固定循环合作→背叛→背叛→合作</li>";
        html += "</ul>";

        html += "<h4 style='margin-top:14px;'>"
                "<span style='font-size:medium; font-weight:700;'>当前设置</span></h4>";
        html += "<ul style='margin-top:0px;'>";
        html += "<li><b>资产继承</b>：" + QString::number(scoreInheritPct) + "%（每轮后积分按此比例折算）</li>";
        html += "<li><b>失误率</b>：" + QString::number(errorPct) + "%</li>";
        html += "<li><b>淘汰</b>：每 " + QString::number(elimInt) + " 轮淘汰 " + QString::number(elimCnt) + " 人</li>";
        html += "<li><b>继承历史</b>：" + QString(inheritHistory ? "启用" : "禁用") + "</li>";
        html += "<li><b>自动演化</b>：默认 " + QString::number(evoRounds) + " 回合</li>";
        html += "</ul>";
    }

    return html;
}
