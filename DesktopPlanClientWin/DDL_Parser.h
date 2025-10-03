//
// Created by Jiang_Boyuan on 25-10-3.
//

#ifndef DDL_PARSER_H
#define DDL_PARSER_H

#include <QString>
#include <QDateTime>

QDate uncertain_ddl_to_certain_ddl(QString);
int certain_ddl_to_rest_day(QDate);
QString rest_day_to_desc(int);

class DDL_Parser {

};



#endif //DDL_PARSER_H
