#ifndef _EAGLEEYE_MysqlNode_H_
#define _EAGLEEYE_MysqlNode_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include <vector>
#include <map>
#include <memory>
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"
#include "eagleeye/framework/pipeline/StringSignal.h"
#include "eagleeye/framework/pipeline/JsonSignal.h"
#include "eagleeye/common/CJsonObject.hpp"
#include "eagleeye/common/EagleeyeStr.h"
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>
#include <ctime> // 包含 time_t, struct tm, strftime 等

namespace eagleeye{
enum MysqlMode{
    MYSQL_INSERT = 0,
    MYSQL_QUERY = 1,
    MYSQL_REMOVE = 2
}; 

class MysqlNode:public AnyNode, DynamicNodeCreator<MysqlNode>{
public:
    typedef MysqlNode               Self;
    typedef AnyNode                 Superclass;

    EAGLEEYE_CLASSIDENTITY(MysqlNode);

    MysqlNode(){
        m_con = NULL;
    }
    MysqlNode(std::string name, std::string host, std::string user, std::string password){
        m_con = sql::mysql::get_mysql_driver_instance()->connect(host, user, password);
        m_con->setSchema(name);
    
        this->m_db_name = name;
        this->m_db_host = host;
        this->m_db_username = user;
        this->m_db_password = password;

        this->setNumberOfInputSignals(1);
        this->setNumberOfOutputSignals(1);
        this->setOutputPort(new BooleanSignal(), 0);        // 执行是否成功
    }
    virtual ~MysqlNode(){
        if(m_con != NULL){
            delete m_con;
        }
    }

    // 接收一个10位的秒级时间戳，返回格式化后的字符串
    std::string formatTimestamp(long long timestamp_seconds) {
        // 1. 将时间戳转换为 time_t 类型
        time_t raw_time = static_cast<time_t>(timestamp_seconds);

        // 2. 将 time_t 转换为 struct tm (本地时间)
        struct tm timeinfo;

        // 使用线程安全的版本，避免使用非线程安全的 localtime()
#if defined(_WIN32) || defined(_WIN64)
        // Windows 平台
        localtime_s(&timeinfo, &raw_time);
#else
        // Linux, macOS (POSIX 平台)
        localtime_r(&raw_time, &timeinfo);
#endif

        // 3. 使用 strftime 进行格式化
        char buffer[80];
        // %Y: 2025, %m: 06, %d: 28, %H: 14, %M: 30, %S: 00
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);

        return std::string(buffer);
    }

    virtual void executeNodeInfo(){
        // input json, [{'table': , 'name': , 'type': , }, {}]
        // INSERT INTO users(name, age) VALUES(?, ?)
        // TOOD，目前仅支持MYSQL_INSERT
        if(m_con == NULL){
            BooleanSignal* status = (BooleanSignal*)this->getOutputPort(0);
            status->setData(false);
            return;
        }
        
        JsonSignal* input_signal = (JsonSignal*)this->getInputPort(0);
        std::string content = input_signal->getData();
        neb::CJsonObject table_info(content);
        for(int i=0; i<table_info.GetArraySize(); ++i){
            neb::CJsonObject item_info;
            table_info.Get(i, item_info);
            
            std::string sql_str;
            std::string table_name;
            item_info.Get("table", table_name);
            sql_str = "INSERT INTO "+table_name+"(";
            neb::CJsonObject field;
            item_info.Get("field", field);
            std::vector<std::string> name_list;
            std::vector<std::string> type_list;
            std::vector<std::string> value_list;
            for(int k=0; k<field.GetArraySize(); ++k){
                neb::CJsonObject filed_info;
                field.Get(k, filed_info);

                std::string name;
                filed_info.Get("name", name);
                name_list.push_back(name);
                std::string type;
                filed_info.Get("type", type);
                type_list.push_back(type);
                std::string value;
                filed_info.Get("value", value);
                value_list.push_back(value);
            }

            for(int k=0; k<name_list.size(); ++k){
                if(k < name_list.size() - 1){
                    sql_str += name_list[k]+", ";
                }
                else{
                    sql_str += name_list[k]+") ";
                }
            }
            sql_str += "VALUES(";
            for(int k=0; k<name_list.size(); ++k){
                if(k < name_list.size() - 1){
                    sql_str += "?, ";
                }
                else{
                    sql_str += "?)";
                }
            }

            try {
                std::unique_ptr<sql::PreparedStatement> pstmt(
                    m_con->prepareStatement(sql_str)
                );

                for(int k=0; k<name_list.size(); ++k){
                    if(type_list[k] == "text"){
                        pstmt->setString(k+1, value_list[k]);
                    }
                    else if(type_list[k] == "int"){
                        pstmt->setInt(k+1, tof<int>(value_list[k]));
                    }
                    else if(type_list[k] == "float"){
                        pstmt->setDouble(k+1, tof<double>(value_list[k]));
                    }
                    else if(type_list[k] == "date"){
                        // 需要自动将时间戳转换为日期格式
                        std::string date_str = formatTimestamp(tof<long long>(value_list[k]));
                        pstmt->setDateTime(k+1, date_str);
                    }
                    else if(type_list[k] == "bool"){
                        pstmt->setBoolean(k+1, tof<bool>(value_list[k]));
                    }
                }

                pstmt->executeUpdate();
            } catch (sql::SQLException &e) {
                std::cerr << "Error adding user: " << e.what() << std::endl;
            }
        }
    }

private:
    std::string m_db_name;
    std::string m_db_host;
    std::string m_db_username;
    std::string m_db_password;
    sql::Connection *m_con;
};
}

#endif