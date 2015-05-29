#include <assert.h>
#include <boost/shared_ptr.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "log/log.h"
#include "receiver.h"
#include "handle_task.h"
#include "utility/net_utility.h"

int main(int argc, char* argv[])
{
    /// load config file
    const char* config_file = "config.xml";
    if(argc < 2)
        LOG_INFO("use default config file: %s\n", config_file);
    else {
        config_file = argv[1];
        LOG_INFO("use config file: %s\n", config_file);
    }
    boost::property_tree::ptree pt;
    read_xml(config_file, pt);

    /// initialize bitmap
    boost::property_tree::ptree bitmap_conf = pt.get_child("root.bitmap");
    size_t count = bitmap_conf.get<size_t>("count");
    size_t order = bitmap_conf.get<size_t>("order");
    std::string save_file_prefix = bitmap_conf.get<std::string>("save_file_prefix");
    time_t save_interval = bitmap_conf.get<time_t>("save_interval");
    if(Bloomfilter::Instance()->initialize(count, order, save_file_prefix, save_interval) < 0)
    {
        LOG_ERROR("Bloomfilter initialize error.\n");
        return -1;
    }

    /// initialize handle task
    int thread_num = pt.get<int>("root.thread_num");
    int max_queue_size = pt.get<int>("root.max_queue_size");
    if(HandleTask::Instance()->initialize(thread_num, max_queue_size) < 0)
    {
        LOG_ERROR("HandleTask initialize error.\n");
        return -1;
    }
    HandleTask::Instance()->open();

    /// initialize httpserver
    std::string listen_port = pt.get<std::string>("root.port");
    std::string listen_eth  = pt.get<std::string>("root.eth");
    std::string listen_ip;
    get_local_address(listen_eth, listen_ip);
    boost::shared_ptr<Receiver> receiver(new Receiver());
    if(receiver->initialize(listen_ip, listen_port) < 0)
    {
        return -1;
    }
    
    receiver->run();

    return 0;
}
