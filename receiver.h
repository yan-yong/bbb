#ifndef __RECV_TASK_H
#define __RECV_TASK_H
#include "handle_task.h"

class Receiver: public HttpServer
{
    virtual void handle_recv_request(boost::shared_ptr<http::server4::request> http_req, 
            boost::shared_ptr<HttpSession> session)
    {
        Request request(http_req, session);
        HandleTask::Instance()->enqueue(request);
    }
}; 

#endif
