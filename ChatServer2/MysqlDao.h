#ifndef MYSQLDAO_H
#define MYSQLDAO_H
#include <queue>
#include <string>
#include <atomic>
#include <mysql_driver.h>
#include <condition_variable>


class MysqlConnPool {
public:
    MysqlConnPool(const std::string& url, const std::string& usr, const std::string& pwd,
         const std::string& schema, std::size_t poolSize);
    ~MysqlConnPool();

    std::unique_ptr<sql::Connection> GetConnection();
    void ReturnConnection(std::unique_ptr<sql::Connection> conn);
    void Close();

private:
    std::atomic<bool> _b_stop;
    std::string _url;
    std::string _usr;
    std::string _pwd;
    std::string _schema;
    std::size_t _poolSize;
    std::queue<std::unique_ptr<sql::Connection>> _pool;
    std::mutex _mutex;
    std::condition_variable _cond;
};

class ApplyInfo;
class UserInfo;
class MysqlDao {
public:
    MysqlDao();
    ~MysqlDao();

    int UserRegister(const std::string& name,
         const std::string& email, const std::string& passwd);
    bool CheckEmail(const std::string& name, const std::string& email);
    bool UpdatePwd(const std::string& name, const std::string& pwd);
    bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userinfo);
    std::shared_ptr<UserInfo> GetUser(int uid);
    std::shared_ptr<UserInfo> GetUser(std::string name);

    bool AddFriendApply(const int& from, const int& to);
    bool AuthFriendApply(const int& from, const int& to);
    bool AddFriend(const int& from, const int& to, std::string backname);
    bool GetApplyList(int touid, 
         std::vector<std::shared_ptr<ApplyInfo>>& apply_list, int begin, int limit = 10);
    bool GetFriendList(int self_id,
         std::vector<std::shared_ptr<UserInfo>>& user_list);
private:
    std::unique_ptr<MysqlConnPool> _pool;
};
#endif // MYSQLDAO_H