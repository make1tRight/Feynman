#include "LogicSystem.h"
#include "const.h"
#include "RedisMgr.h"
#include "HttpConnection.h"
#include "VarifyGrpcClient.h"
#include "StatusGrpcClient.h"
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/reader.h>
#include <MysqlMgr.h>

LogicSystem::LogicSystem() {
    RegisterGet("/get_test", [](std::shared_ptr<HttpConnection> conn) {
        beast::ostream(conn->_response.body()) 
            << "get request received." << std::endl;
        int i = 0;
        for (const auto& elem : conn->_get_params) {
            ++i;
            beast::ostream(conn->_response.body()) 
                << "======Params[" << i << "]======" << std::endl; 
            beast::ostream(conn->_response.body()) << "key: " << elem.first 
                << ", value: " << elem.second << std::endl;
        }
    });

    RegisterPost("/get_varifycode", [](std::shared_ptr<HttpConnection> conn) {
        auto body_str = beast::buffers_to_string(conn->_request.body().data());
        std::cout << "body string: " << body_str << std::endl;
        conn->_response.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Reader reader;
        Json::Value root_src;
        bool success = reader.parse(body_str, root_src);
        if (!success) {
            std::cout << "Failed to parse JSON data." << std::endl;
            root["error"] = ERROR_CODES::JSON_ERROR;
            std::string jsonstr = root.toStyledString();
            beast::ostream(conn->_response.body()) << jsonstr;
            return;
        }
        auto email = root_src["email"].asString();
        GetVarifyRsp rsp = VarifyGrpcClient::GetInstance()->GetVarifyCode(email);
        std::cout << "email: " << email << std::endl;
        root["email"] = email;
        root["error"] = rsp.error();
        std::string jsonstr = root.toStyledString();
        beast::ostream(conn->_response.body()) << jsonstr;
    });

    RegisterPost("/user_register", [](std::shared_ptr<HttpConnection> conn) {
        auto body_str = beast::buffers_to_string(conn->_request.body().data());
        std::cout << "body string: " << body_str << std::endl;
        conn->_response.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;
        bool success = reader.parse(body_str, src_root);
        // json有误
        if (!success) {
            std::cout << "Failed to parse JSON data." << std::endl;
            root["error"] = ERROR_CODES::JSON_ERROR;
            std::string jsonstr = root.toStyledString();
            beast::ostream(conn->_response.body()) << jsonstr;
            return;
        }
        std::string email = src_root["email"].asString();
        std::string name = src_root["user"].asString();
        std::string passwd = src_root["passwd"].asString();
        std::string confirm = src_root["confirm"].asString();
        if (passwd != confirm) {
            std::cout << "password error." << std::endl;
            root["error"] = ERROR_CODES::PASSWD_ERR;
            std::string jsonstr = root.toStyledString();
            beast::ostream(conn->_response.body()) << jsonstr;
            return;
        }

        std::string varifycode = "";
        // 验证码过期
        bool b_get_varifycode = RedisMgr::GetInstance()->Get(CODEPREFIX + email, varifycode);
        if (!b_get_varifycode) {
            std::cout << "Code expired." << std::endl;
            root["error"] = ERROR_CODES::VARIFY_EXPIRED;
            std::string jsonstr = root.toStyledString();
            beast::ostream(conn->_response.body()) << jsonstr;
            return;
        }
        // 验证码错误
        if (varifycode != src_root["varifycode"].asString()) {
            std::cout << "Code error." << std::endl;
            root["error"] = ERROR_CODES::VARIFY_CODE_ERROR;
            std::string jsonstr = root.toStyledString();
            beast::ostream(conn->_response.body()) << jsonstr;
            return;
        }
        
        // 用户已存在
        int uid = MysqlMgr::GetInstance()->UserRegister(name, email, passwd);
        if (uid == 0 || uid == -1) {
            std::cout << "User exists." << std::endl;
            root["error"] = ERROR_CODES::USER_EXIST;
            std::string jsonstr = root.toStyledString();
            beast::ostream(conn->_response.body()) << jsonstr;
            return;
        }

        root["error"] = 0;
        root["uid"] = uid;
        root["email"] = email;
        root["user"] = name;
        root["passwd"] = passwd;
        root["confirm"] = confirm;
        root["varifycode"] = src_root["varifycode"].asString();
        std::string jsonstr = root.toStyledString();
        beast::ostream(conn->_response.body()) << jsonstr;
        return;
    });

    RegisterPost("/reset_pwd", [](std::shared_ptr<HttpConnection> conn) {
        auto body_str = beast::buffers_to_string(conn->_request.body().data());
        std::cout << "body str: " << body_str << std::endl;
        conn->_response.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;
        bool parse_success = reader.parse(body_str, src_root);
        if (!parse_success) {
            std::cout << "Failed to parse JSON data." << std::endl;
            root["error"] = ERROR_CODES::JSON_ERROR;
            std::string jsonstr = root.toStyledString();
            beast::ostream(conn->_response.body()) << jsonstr;
            return;
        }

        std::string varifycode;
        std::string email = src_root["email"].asString();
        bool b_get_varifycode = RedisMgr::GetInstance()->Get(CODEPREFIX + email, varifycode);
        if (!b_get_varifycode) {
            std::cout << "Get varify code error." << std::endl;
            root["error"] = ERROR_CODES::VARIFY_EXPIRED;
            std::string jsonstr = root.toStyledString();
            beast::ostream(conn->_response.body()) << jsonstr;
            return;
        }
        if (varifycode != src_root["varifycode"].asString()) {
            std::cout << "Varify code error." << std::endl;
            root["error"] = ERROR_CODES::VARIFY_CODE_ERROR;
            std::string jsonstr = root.toStyledString();
            beast::ostream(conn->_response.body()) << jsonstr;
            return;
        }

        std::string name = src_root["user"].asString();
        bool email_valid = MysqlMgr::GetInstance()->CheckEmail(name, email);
        if (!email_valid) {
            std::cout << "User email not match" << std::endl;
            root["error"] = ERROR_CODES::EMAIL_NOT_MATCH;
            std::string jsonstr = root.toStyledString();
            beast::ostream(conn->_response.body()) << jsonstr;
            return;
        }

        std::string passwd = src_root["passwd"].asString();
        bool b_update = MysqlMgr::GetInstance()->UpdatePwd(name, passwd);
        if (!b_update) {
            std::cout << "Failed to update password" << std::endl;
            root["error"] = ERROR_CODES::PWD_UPDATE_FAILED;
            std::string jsonstr = root.toStyledString();
            beast::ostream(conn->_response.body()) << jsonstr;
            return;
        }
        
        std::cout << "Update password success" << std::endl;
        root["error"] = 0;
        root["user"] = name;
        root["email"] = email;
        root["passwd"] = passwd;
        root["varifycode"] = src_root["varifycode"].asString();
        std::string jsonstr = root.toStyledString();
        beast::ostream(conn->_response.body()) << jsonstr;
    });

    RegisterPost("/user_login", [](std::shared_ptr<HttpConnection> conn) {
        std::string body_str = beast::buffers_to_string(conn->_request.body().data());
        std::cout << "body_str: " << body_str << std::endl;
        conn->_response.set(http::field::content_type, "text/json");

        Json::Value src_root;
        Json::Reader reader;
        Json::Value root;
        bool parse_success =  reader.parse(body_str, src_root);
        if (!parse_success) {
            std::cout << "Failed to parse JSON data." << std::endl;
            root["error"] = ERROR_CODES::JSON_ERROR;
            std::string jsonstr = root.toStyledString();
            beast::ostream(conn->_response.body()) << jsonstr;
            return;
        }

        std::string email = src_root["email"].asString();
        std::string passwd = src_root["passwd"].asString();
        UserInfo userinfo;
        bool pwd_valid =  MysqlMgr::GetInstance()->CheckPwd(email, passwd, userinfo);
        if (!pwd_valid) {
            std::cout << "Email and password do not match." << std::endl;
            root["error"] = ERROR_CODES::PASSWD_INVALID;
            std::string jsonstr = root.toStyledString();
            beast::ostream(conn->_response.body()) << jsonstr;
            return;
        }
        
        auto reply = StatusGrpcClient::GetInstance()->GetChatServer(userinfo._uid);
        if (reply.error()) {
            std::cout << "Failed to get ChatServer by gRPC, error: " 
                << reply.error() << std::endl;
            root["error"] = ERROR_CODES::RPC_FAILED;
            std::string jsonstr = root.toStyledString();
            beast::ostream(conn->_response.body()) << jsonstr;
            return;
        }
        std::cout << "Success to load userinfo, uid: " << userinfo._uid << std::endl;
        root["error"] = 0;
        root["email"] = email;
        root["uid"] = userinfo._uid;
        root["token"] = reply.token();
        root["host"] = reply.host();
        root["port"] = reply.port();
        std::string jsonstr = root.toStyledString();
        beast::ostream(conn->_response.body()) << jsonstr;
        return;
    });
}

LogicSystem::~LogicSystem() {

}

bool LogicSystem::HandleGet(std::string path,
    std::shared_ptr<HttpConnection> conn) {
    if (_get_handler.find(path) == _get_handler.end()) {
        return false;
    }

    _get_handler[path](conn);
    return true;
}

bool LogicSystem::HandlePost(std::string path,
    std::shared_ptr<HttpConnection> conn) {
    if (_post_handler.find(path) == _post_handler.end()) {
        return false;
    }

    _post_handler[path](conn);
    return true;
}

void LogicSystem::RegisterGet(std::string url, HttpHandler handler) {
    _get_handler.insert(std::make_pair(url, handler));
}

void LogicSystem::RegisterPost(std::string url, HttpHandler handler) {
    _post_handler.insert(std::make_pair(url, handler));
}
