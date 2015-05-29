#include "bloomfilter.h"
#include <boost/bind.hpp>

DEFINE_SINGLETON(Bloomfilter);
Bloomfilter::Bloomfilter(): 
    m_order(0), m_count(0), m_save_interval(0), m_exit(false), 
    m_bitmap_array(NULL), m_thread("bloomfilter_save", false)
{
}

Bloomfilter::~Bloomfilter()
{
    exit();
    for(unsigned i = 0; i < m_count; i++)
        if(m_bitmap_array[i]){
            delete m_bitmap_array[i];
            m_bitmap_array[i] = NULL;
        }
    if(m_bitmap_array){
        delete[] m_bitmap_array;
        m_bitmap_array = NULL;
    }
}

void Bloomfilter::__save_runtine()
{
    time_t save_time = time(NULL);
    while(!m_exit)
    {
        time_t cur_time = time(NULL);
        //prevent date time change
        if(cur_time < save_time)
            save_time = cur_time;
        if(cur_time - save_time >= m_save_interval) {
            for(unsigned i = 0; i < m_count && !m_exit; i++)
                DenseBitmap::save_routine(m_bitmap_array[i]);
            save_time = cur_time;
        }
        if(!m_exit)
            sleep(1);
    }
}

int Bloomfilter::initialize(size_t count, size_t order, 
            std::string save_file_prefix, time_t save_interval)
{
    m_count = count;
    m_order = order;
    m_save_file_prefix = save_file_prefix;
    m_save_interval = save_interval;

    m_bitmap_array = new DenseBitmap*[count];
    for(unsigned i = 0; i < m_count; i++) 
    {
        char file_name[1024];
        snprintf(file_name, 1024, "%s_%u.dat", save_file_prefix.c_str(), i);
        m_bitmap_array[i] = new DenseBitmap();
        if(m_bitmap_array[i]->initialize(order, std::string(file_name)) < 0)
            return -1;
    }
    m_thread.set_runtine(boost::bind(&Bloomfilter::__save_runtine, this));
    m_thread.open();
    return 0;
}

void Bloomfilter::exit()
{
    if(!__sync_bool_compare_and_swap(&m_exit, false, true))
        return;
    for(unsigned i = 0; i < m_count; i++)
        m_bitmap_array[i]->exit();
    m_thread.exit();
}
