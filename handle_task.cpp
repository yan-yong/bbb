#include "handle_task.h"
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <exception>

#define PRE_BUFFER_SIZE 6*1024*1024
#define RESPONSE_COMPRESS_SIZE 1024*2 

#define JSON_NAME_LINK "lk"
#define JSON_NAME_OP "op"
#define JSON_NAME_STATUS "st"
#define JSON_VALUE_GET "g"
#define JSON_VALUE_SET "s"
#define JSON_VALUE_INSERT "i" 
#define JSON_VALUE_UNSET "u"
#define JSON_VALUE_EXIST "1"
#define JSON_VALUE_NOT_EXIST "0"

DEFINE_SINGLETON(HandleTask);

HandleTask::HandleTask()
{
}

HandleTask::ErrCode HandleTask::__reply_error(Request& req, HandleTask::ErrCode code)
{
    assert(code < ERROR_NUM && code >= 0);
    static const char* err_msg[] = {
        "ok",
        "gzip error",
        "json format error",
        "write file error",
        "empty get request",
        "process json content error"
    };
    if(code == WRITE_FILE_ERROR)
        req.m_session->m_reply->status  = http::server4::reply::internal_server_error;
    else
        req.m_session->m_reply->status  = http::server4::reply::bad_request;
    req.m_session->m_reply->content = err_msg[code];
    req.m_session->send_response();
    return OK;
}

HandleTask::ErrCode HandleTask::__reply_ok(Request& req, const std::string& response_content, 
        const std::vector<http::server4::header>& headers)
{
    req.m_session->m_reply->status  = http::server4::reply::ok;
    req.m_session->m_reply->content = response_content;
    req.m_session->m_reply->headers = headers;
    req.m_session->send_response();
    return OK;         
}

int HandleTask::__decompress_content(std::string& content, std::vector<char> & buffer)
{
    //ensure content length
    if(gzdecompress(content.c_str(), content.size(), buffer))
        return -1;
    content.assign(&buffer[0], buffer.size());
    if(buffer.capacity() > PRE_BUFFER_SIZE){
        std::vector<char> tmp_vec(PRE_BUFFER_SIZE);
        buffer.swap(tmp_vec);
    }
    return 0;
}

int HandleTask::__compress(std::string& response_content, std::vector<char> & buffer)
{
    if(gzcompress(response_content.c_str(), response_content.size(), buffer))
    {
        LOG_ERROR("gzip compress error: %s\n", response_content.substr(0, 100).c_str());
        return -1;
    }
    response_content.assign(&buffer[0], buffer.size());
    if(buffer.capacity() > PRE_BUFFER_SIZE){
        std::vector<char> tmp_vec(PRE_BUFFER_SIZE);
        buffer.swap(tmp_vec);
    }
    return 0;
}

HandleTask::ErrCode HandleTask::__process_content(Request& request, std::string& response_content)
{
    std::string & content = request.m_http_req->content;
    std::string ip   = request.m_session->ip();
    std::string port = request.m_session->port();
    Json::Reader reader;
    Json::Value input_json, output_json;
    if(!reader.parse(content, input_json)) {
        LOG_ERROR("invalid json content: %s:%s %s, content_size: %zd\n", 
                request.m_session->ip().c_str(), request.m_session->port().c_str(), 
                content.c_str(), content.size());
        return JSON_FORMAT_ERROR;
    }
    for(unsigned i = 0; i < input_json.size(); i++){
        std::string link = input_json[i][JSON_NAME_LINK].asString();
        if(link.empty()){
            LOG_ERROR("skip empty lk: %s:%s %s\n", ip.c_str(), port.c_str(),
                    input_json[i].toStyledString().c_str());
            continue;
        }
        std::string op = input_json[i][JSON_NAME_OP].asString();
        if(op.empty()){
            LOG_ERROR("empty op: %s:%s %s\n", ip.c_str(), port.c_str(),
                    input_json[i].toStyledString().c_str());
            continue;
        }
        if(op == JSON_VALUE_SET){
            if(Bloomfilter::Instance()->set(link) < 0){
                LOG_ERROR("write file failed: %s:%s %s\n", ip.c_str(), port.c_str(),
                        input_json[i].toStyledString().c_str());
                return WRITE_FILE_ERROR;
            }
        }
        else if(op == JSON_VALUE_UNSET){
             if(Bloomfilter::Instance()->unset(link) < 0){
                LOG_ERROR("write file failed: %s:%s %s\n", ip.c_str(), port.c_str(),
                        input_json[i].toStyledString().c_str());
                return WRITE_FILE_ERROR;
            }
        }
        else if(op == JSON_VALUE_GET){
            Json::Value cur_val;
            cur_val["lk"] = link;
            cur_val["st"] = boost::lexical_cast<std::string>(Bloomfilter::Instance()->get(link));
            output_json.append(cur_val);
        }
        else if(op == JSON_VALUE_INSERT){
            Json::Value cur_val;
            cur_val["lk"] = link;
            int ret = Bloomfilter::Instance()->set(link); 
            if(ret < 0) 
            {
                LOG_ERROR("write file failed: %s:%s %s\n", ip.c_str(), port.c_str(),
                        input_json[i].toStyledString().c_str());
                return WRITE_FILE_ERROR;
            }
            cur_val["st"] = boost::lexical_cast<std::string>(ret);
            output_json.append(cur_val);       
        }
        else{
            LOG_ERROR("invalid op: %s:%s %s\n", ip.c_str(), port.c_str(),
                    input_json[i].toStyledString().c_str());
        }
    }
    response_content = output_json.toStyledString();
    return OK;
}

void HandleTask::run()
{
    std::vector<char> buffer;
    Request request;
    while(m_queue.dequeue(request))
    {
        std::string ip   = request.m_session->ip();
        std::string port = request.m_session->port();
        std::string content_encode, accept_encode;
        std::vector<std::string> content_encode_vec = request.m_http_req->get_header("Content-Encoding");
        if(content_encode_vec.size() > 0)
            content_encode = content_encode_vec[0];
        std::vector<std::string> accept_encode_vec = request.m_http_req->get_header("Accept-Encoding");
        if(accept_encode_vec.size() > 0)
            accept_encode = accept_encode_vec[0];

        /// decompress
        if(content_encode.find("gzip") != std::string::npos && 
                __decompress_content(request.m_http_req->content, buffer) < 0)
        {
            LOG_ERROR("gzip decompress error: %s:%s\n", ip.c_str(), port.c_str());
            __reply_error(request, GZIP_ERROR);
            continue;
        }

        /// set bloomfilter
        std::string response_content;
        HandleTask::ErrCode code = OK;
        try{
            code = __process_content(request, response_content);
        }catch(std::exception& err){
            code = PROCESS_JSON_ERROR; 
            response_content = err.what();
            LOG_ERROR("process json exception: %s %s:%s\n", err.what(), ip.c_str(), port.c_str());
        }
        if(code != OK){
            __reply_error(request, code);
            continue;
        }

        ///  compress && response
        size_t src_data_len = response_content.size();
        if(accept_encode.find("gzip") != std::string::npos && response_content.size() >= RESPONSE_COMPRESS_SIZE
                && __compress(response_content, buffer) == 0)
        {
            LOG_DEBUG("compress data [%zd/%zd]: %s:%s\n", src_data_len, response_content.size(), 
                    ip.c_str(), port.c_str());
            http::server4::header tmp_header;
            tmp_header.name  = "Content-Encoding";
            tmp_header.value = "gzip";
            request.m_session->m_reply->headers.push_back(tmp_header);
        }
        request.m_session->m_reply->status = http::server4::reply::ok;
        request.m_session->m_reply->content= response_content;
        request.m_session->send_response();
    }
}
