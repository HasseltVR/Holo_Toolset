 /**********************************************************
 * Holo_ToolSet 
 * HPV Creator Console version
 *
 * http://github.com/HasseltVR/Holo_ToolSet
 * http://www.uhasselt.be/edm
 *
 * Distributed under LGPL v2.1 Licence
 * http ://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 **********************************************************/

#include <iostream>
#include <vector>
#include <string>
#include <sstream>

#include <stdio.h>
#include <dirent.h>

#include "cmdline.h"
#include "HPVCreator.hpp"

using namespace HPV;

/******************************************************************************
 * Private state.
 ******************************************************************************/

static HPVCreatorParams hpv_params;
static HPVCreator hpv_creator;
static std::vector<std::string> file_names;
static std::string m_cur_filename;
static uint32_t planned_total;
static ThreadSafe_Queue<HPVCompressionProgress> progress_sink;
static HPVCompressionProgress progress;


/******************************************************************************
 * Private methods.
 ******************************************************************************/

static void setup_parser(cmdline::parser& p)
{
    p.add<std::string>("in", 'i', "in path", true);
    p.add<int>("fps", 'f', "framerate", true);
    p.add<int>("type", 't', "type", true);
    
    p.add<int>("start", 's', "start frame", false, 0);
    p.add<int>("end", 'e', "end frame", false, 100);
    p.add<std::string>("out", 'o', "out path", false, "");
    p.add<int>("threads", 'n', "num threads", false, 8);
}

static uint32_t parse_in_path(HPVCreatorParams& params)
{
    uint32_t count = 0;
    struct dirent *de;
    std::string filename;
    const std::string& path = params.in_path;
    
    DIR *dir = opendir(path.c_str());
    if(!dir)
    {
        return 0;
    }
    
    while ((de = readdir(dir)) != NULL)
    {
        std::string fname(de->d_name);
        if (HPV::file_supported(fname))
        {
            count++;
            
            filename.append(hpv_params.in_path);
            filename.append("/");
            filename.append(de->d_name);
            file_names.push_back(filename);
            filename.clear();
        }
    }
    
    params.file_names = &file_names;
    
    closedir(dir);
    
    return count;
}

static bool parse_params(const cmdline::parser& p)
{
    hpv_params.in_path = p.get<std::string>("in");
    hpv_params.out_path = p.get<std::string>("out");
    hpv_params.fps = p.get<int>("fps");
    hpv_params.type = static_cast<HPVCompressionType>(p.get<int>("type"));
    if (hpv_params.type >= HPVCompressionType::HPV_NUM_TYPES)
    {
        hpv_params.type = HPVCompressionType::HPV_TYPE_DXT1_NO_ALPHA;
    }
    hpv_params.num_threads = p.get<int>("threads");
    
    if ((planned_total = parse_in_path(hpv_params)) == 0)
    {
        HPV_ERROR("In path doesn't exist");
        return false;
    }
    
    if (p.exist("start"))
    {
        hpv_params.in_frame = p.get<int>("start");
    }
    else
    {
        hpv_params.in_frame = 0;
    }
    
    if (p.exist("end"))
    {
        hpv_params.out_frame = p.get<int>("end");
    }
    else
    {
        hpv_params.out_frame = planned_total-1;
    }
    
    if (p.exist("out"))
    {
        hpv_params.out_path = p.get<std::string>("out");
    }
    else
    {
        hpv_params.out_path = hpv_params.in_path;
        hpv_params.out_path.append("/out.hpv");
    }
    
    return true;
}


static bool parse_progress()
{
    if (progress_sink.try_pop(progress))
    {
        switch (progress.state)
        {
            case HPV_CREATOR_STATE_ERROR:
                HPV_ERROR("%s", progress.done_item_name.c_str());
                hpv_creator.stop();
                return false;
            case HPV_CREATOR_STATE_DONE:
                HPV_VERBOSE("%s", progress.done_item_name.c_str());
                hpv_creator.stop();
                return false;
            default:
            {
                std::stringstream ss;
                int percent = static_cast<int>( (progress.done_items/(float)progress.total_items) * 100);
                
                ss  << "["
                << percent
                << "%]: "
                << progress.done_item_name
                << " [deflated to: "
                << progress.compression_ratio
                << "%]";
                
                HPV_VERBOSE("%s", ss.str().c_str());
                return true;
            }
        }
    }
    
    return true;
}


/******************************************************************************
 * Main application.
 ******************************************************************************/

int main(int argc, char *argv[])
{
    HPV::hpv_log_init();
    HPV::hpv_log_enable_stdout();
    
    cmdline::parser p;
    
    setup_parser(p);
    p.parse_check(argc, argv);
    parse_params(p);
    
    if (hpv_creator.init(hpv_params, &progress_sink) == HPV_RET_ERROR)
    {
        parse_progress();
        return 1;
    }
    
    hpv_creator.process_sequence(hpv_params.num_threads);
    
    while (parse_progress()) {}
    
    return 0;
}
