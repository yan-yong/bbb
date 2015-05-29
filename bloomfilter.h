#ifndef __BLOOM_FILTER_H
#define __BLOOM_FILTER_H

#include <string>
#include "httpserver/httpserver.h"
#include "bitmap/DenseBitmap.h"
#include "thread/Thread.h"
#include "singleton/Singleton.h"

class Bloomfilter
{
    size_t m_order;
    size_t m_count;
    time_t m_save_interval;
    bool m_exit;
    std::string m_save_file_prefix;
    DenseBitmap** m_bitmap_array; 
    CThread m_thread;

    void __save_runtine();
    size_t __get_hash_data(size_t data, int index)
    {
        return data + index;  
    }
    std::string __get_hash_data(const std::string& data, int index)
    {
        char after[20];
        snprintf(after, 20, "-%u", index);
        return data + after; 
    }
public:
    ~Bloomfilter();
    int initialize( size_t count, size_t order, 
            std::string save_file, time_t save_interval);
    void exit();

    int set(const std::string & data)
    {
        int ret = 1;
        for(unsigned i = 0; i < m_count; i++)
        {
            int cur_ret = m_bitmap_array[i]->set(__get_hash_data(data, i));
            if(cur_ret < 0)
                return -1;
            ret &= cur_ret;
        }
        return ret;
    }
    int set(size_t data)
    {
        int ret = 0;
        for(unsigned i =0; i < m_count; i++)
        {
            int cur_ret = m_bitmap_array[i]->set(__get_hash_data(data, i));
            if(cur_ret < 0)
                return -1;
            ret &= cur_ret;
        }
        return ret;
    }
    int get(const std::string & data)
    {
        for(unsigned i = 0; i < m_count; i++)
            if(m_bitmap_array[i]->get(__get_hash_data(data, i)) == 0)
                return 0;
        return 1;
    }
    int get(size_t data)
    {
        for(unsigned i =0; i < m_count; i++)
            if(m_bitmap_array[i]->get(__get_hash_data(data, i)) == 0)
                return 0;
        return 1;
    }
    int unset(const std::string & data)
    {
        for(unsigned i = 0; i < m_count; i++)
            if(m_bitmap_array[i]->unset(__get_hash_data(data, i)) < 0)
                return -1;
        return 0;
    }
    int unset(size_t data)
    {
        for(unsigned i =0; i < m_count; i++)
            if(m_bitmap_array[i]->unset(__get_hash_data(data, i)) < 0)
                return -1;
        return 0;
    }

    DECLARE_SINGLETON(Bloomfilter);
};
#endif
