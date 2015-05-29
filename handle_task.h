#ifndef __HANDLE_TASK_H 
#define __HANDLE_TASK_H

#include "thread/DataPool.h"
#include "bloomfilter.h"
#include "singleton/Singleton.h"
#include "httpserver/httpserver.h"
#include "gzip/gzip.h"
#include "jsoncpp/include/json/json.h"
#include <boost/lexical_cast.hpp>
#include "singleton/Singleton.h"
#include <boost/shared_ptr.hpp>

struct Request
{
    boost::shared_ptr<http::server4::request> m_http_req;
    boost::shared_ptr<HttpSession> m_session;

    Request(boost::shared_ptr<http::server4::request> req, boost::shared_ptr<HttpSession> session):
        m_http_req(req), m_session(session)
    {}

    Request()
    {}
};

class HandleTask: public DataPool<Request>
{
    enum ErrCode
    {
        OK, 
        GZIP_ERROR,
        JSON_FORMAT_ERROR,
        WRITE_FILE_ERROR,
        EMPTY_GET_ERROR,
        PROCESS_JSON_ERROR,
        ERROR_NUM
    };

    ErrCode __reply_error(Request& req, ErrCode code);
    ErrCode __reply_ok(Request& req, const std::string& response_content, const std::vector<http::server4::header>&);
    int __decompress_content(std::string& content, std::vector<char> & buffer);
    int __compress(std::string& response_content, std::vector<char> & buffer);
    ErrCode __process_content(Request& request, std::string& response_content);
    
    //return -1: failed
public:
    virtual void run();

    DECLARE_SINGLETON(HandleTask);
};

#endif
