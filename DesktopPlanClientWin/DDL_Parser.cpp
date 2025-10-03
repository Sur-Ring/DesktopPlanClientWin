//
// Created by Jiang_Boyuan on 25-10-3.
//

#include "DDL_Parser.h"

#include <QDateTime>
#include <QStringList>

QDate uncertain_ddl_to_certain_ddl(QString uncertain_ddl) {
    QStringList parts = uncertain_ddl.split("-");
    if (parts.size() != 3) {
        return {};
    }

    QDate now = QDate::currentDate();

    int year = now.year();
    int month = now.month();
    int day = now.day();

    int pattern_case = 0;

    if(parts[0] == "*") {
        pattern_case += 4;
    }else {
        bool ok=false;
        year = parts[0].toInt(&ok);
        if (!ok) return {};
        if (year>=10000 || year<=2000) return {};
    }

    if(parts[1] == "*") {
        pattern_case += 2;
    }else {
        bool ok=false;
        month = parts[1].toInt(&ok);
        if (!ok) return {};
    }

    if(parts[2] == "*") {
        pattern_case += 1;
    }else {
        bool ok=false;
        day = parts[2].toInt(&ok);
        if (!ok) return {};
    }

    // 000. a-b-c
    if(pattern_case == 0) {}
    // 001, a-b-* 如果是今年本月, 为当天, 未来为1, 过去为最后一天
    else if(pattern_case == 1) {
        if(year==now.year()){ // 如果是今年
            if(month==now.month()){ // 如果是本月为当天
                day = now.day();
            }else if(month>now.month()){ // 未来为1
                day = 1;
            }else{ // 过去为当月最后一天
                day = now.daysInMonth();
            }
        }else if(year>now.year()){ // 未来则为1日
            day = 1;
        }else{ // 过去为当月最后一天
            day = now.daysInMonth();
        }
    }
    // 010. a-*-c 如果是今年, 本月未过则为当月, 否则为下月, 如果当前为12月则为12月, 未来为1月, 过去为12月
    else if(pattern_case == 2) {
        if(year==now.year()){ // 如果是今年
            if(now.day()>day){ // 本月已过为下月, 最高12月
                month = now.month()+1;
                if(month==13)
                    month=12;
            }else{ // 本月未过为本月
                month = now.month();
            }
        }else if(year>now.year()){ // 未来则为1月
            month = 1;
        }else{ // 过去为12月
            month = 12;
        }
    }
    // 011, a-*-*
    else if(pattern_case == 3) {
        if(year==now.year()){ // 如果是今年则为当天
            month = now.month();
            day = now.day();
        }else if(year>=now.year()){ // 未来则为1月1日
            month = 1;
            day = 1;
        }else{ // 过去为12月31日
            month = 12;
            day = 31;
        }
    }
    // 100, *-b-c
    else if(pattern_case == 4) {
        // 如果本年b月c日已过则为明年
        QDate tmp(now.year(),month,day);
        if(tmp < now) {
            year = now.year()+1;
        }else{ // 未过为本年
            year = now.year();
        }
    }
    // 101, *-b-*
    else if(pattern_case == 5) {
        if(now.month()==month){ // 处于b月, 当天
            year = now.year();
            day = now.day();
        }else if(now.month()<month){ // 本年c月未到, 为本年c月1日
            year = now.year();
            day = 1;
        }else{ // 本年c月已过, 为明年c月1日
            year = now.year()+1;
            day = 1;
        }
    }
    // 110, *-*-c
    else if(pattern_case == 6) {
        if(now.day()>day){ // 本月c日已过, 调整到下月
            year = now.addMonths(1).year();
            month = now.addMonths(1).month();
        }else{ // 本月c日未过
            year = now.year();
            month = now.month();
        }
    }
    // 111, *-*-*
    else if(pattern_case == 7) {
        year=now.year();
        month=now.month();
        day=now.day();
    }

    return {year,month,day};
}

int certain_ddl_to_rest_day(QDate certain_ddl) {
    if (!certain_ddl.isValid()) return INT_MIN;
    return QDate::currentDate().daysTo(certain_ddl);
}

QString rest_day_to_desc(int rest_day) {
    if(rest_day == INT_MIN)
        return "无效日期";

    if (rest_day == 0) {
        return "今天";
    }
    if (rest_day == 1) {
        return "明天";
    }
    if (rest_day == 2) {
        return "后天";
    }

    return QString::number(rest_day)+"天";
}