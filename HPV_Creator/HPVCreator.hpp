/**********************************************************
* Holo_ToolSet
* http://github.com/HasseltVR/Holo_ToolSet
* http://www.uhasselt.be/edm
*
* Distributed under LGPL v2.1 Licence
* http ://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
**********************************************************/
#ifndef HPV_CREATOR_H
#define HPV_CREATOR_H

#ifdef _WIN32
#define NOMINMAX
#endif

#include <stdint.h>
#include <fstream>
/* for checking existence of files using stat() */
#ifdef _WIN32
#include <sys/types.h>
#elif defined(__APPLE__) || defined(__APPLE_CC__)
#include <sys/stat.h>
#endif
// TODO INCLUDE LINUX

#include <memory>
#include <thread>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <atomic>
#include <memory>
#include <sstream>

#include "ThreadSafeContainers.hpp"
#include "Timer.h"
#include "Log.hpp"
#include "HPVHeader.hpp"
#include "YCoCg.h"
#include "YCoCgDXT.h"
#include "lz4.h"
#include "lz4hc.h"

#define PROCESSED_SIZE_BARRIER 5
#define PROCESSED_BARRIER_SLEEPTIME 250

#define HPV_CREATOR_STATE_ERROR 0x01
#define HPV_CREATOR_STATE_DONE  0x02
#define HPV_CREATOR_STATE_BUSY  0x03

#ifndef _MSC_VER
/* Template for std::make_unique */
namespace std {
    template<typename T, typename ...Args>
    std::unique_ptr<T> make_unique( Args&& ...args )
    {
        return std::unique_ptr<T>( new T( std::forward<Args>(args)... ) );
    }
}
#endif

namespace HPV {
	
	/*
	* All Supported file types
	*
	*/
    const std::vector<std::string> supported_filetypes = { "*.png", "*.jpeg", "*.jpg", "*.tga", "*.gif", "*.bmp", "*.psd", "*.gif", "*.hdr", "*.pic", "*.ppm", "*.pgm" };
    
	struct HPVCreatorParams
	{
        std::string in_path;
		std::string out_path;
        std::vector<std::string> * file_names;
		uint32_t in_frame;
		uint32_t out_frame;
		uint8_t fps;
		HPVCompressionType type;
	};

    class HPVCompressionWorkItem
    {
    public:
		HPVCompressionWorkItem()
        {
            path = "";
            offset = 0;
        }
		HPVCompressionWorkItem(const std::string& _path, uint64_t _offset) : path(_path) ,offset(_offset) {}
        
        std::string path;
        uint64_t offset;
    };
    
    class HPVCompressedItem
    {
    public:
		HPVCompressedItem()
        {
            write_out_buf = nullptr;
            write_pos = 0;
            frame_size = 0;
            path = "";
        }
        char * write_out_buf;
        uint64_t write_pos;
        uint64_t frame_size;
        std::string path;
        float compression_ratio;
    };

    class HPVCompressionProgress
    {
    public:
        HPVCompressionProgress()
        {
            total_items = 0;
            done_items = 0;
            done_item_name = "";
            compression_ratio = 0;
        }
        uint8_t state;
        int32_t total_items;
        int32_t done_items;
        std::string done_item_name;
        float compression_ratio;
    };
    
	/*
	*	This class will take care of writing all data to the hard drive. We will have one instantation 
	*	of this class that will be used throughout the different threads. For this reason, the write 
	*	operation is secured by means of a std::mutex encapsulated in a std::lock_guard. When the write
	*	operation has completed, the mutex will automatically be unlocked so another thread can start
	*	writing to the stream.
	*
	*/
	class HPVFileStreamWriter
	{
	private:
		std::unique_ptr<std::ofstream> ofs;
		bool bInit;

	public:
		HPVFileStreamWriter() : bInit(false) {}

		~HPVFileStreamWriter() 
		{ 
			if (ofs->is_open()) ofs->close(); 

			ofs.reset();
		}

		int init(const std::string& _path)
		{
            ofs = std::make_unique<std::ofstream>();
			ofs->open(_path.c_str(), std::ios::binary | std::ios::out);

            bInit = (is_good() == 1) ? true : false;

			return bInit;
		}

		int is_good()
		{
			return (ofs->is_open() && ofs->good() && !ofs->fail());
		}
        
        /*
         * Write a Compressed item to disk, already contains all info
         */
        int write_to_stream(const std::shared_ptr<HPVCompressedItem>& item)
        {
            bool bOK = true;
			if (bInit)
			{
				//DXT_VERBOSE("Writing %d bytes to pos %d", item->frame_size, (item->write_pos-40)/item->frame_size);

				// write operation
				if (is_good())
				{
					//uint64_t b = ns();
					ofs->seekp(item->write_pos);
					//uint64_t a = ns();
					//DXT_VERBOSE("Seeking took: %3.2f ms", (a - b) / 1e6);
				}
				else 
					bOK = false;

				if (is_good())
				{
					//uint64_t b = ns();
                    ofs->write((const char *)item->write_out_buf, item->frame_size);
					//uint64_t a = ns();
					//DXT_VERBOSE("Writing took: %3.2f ms", (a - b) / 1e6);
				}
				else 
					bOK = false;

				if (is_good())
				{
					//uint64_t b = ns();
					ofs->flush();
					//uint64_t a = ns();
					//DXT_VERBOSE("Flushing took: %3.2f ms", (a - b) / 1e6);
				}
				else 
					bOK = false;

				if (!bOK)
					HPV_ERROR("Error writing frame @ pos %d", item->write_pos);
			}
			else
			{
				HPV_ERROR("Writing frame to non existing stream.");
                bOK = false;
			}

            return bOK;
        }

        /*
         * Manually pass information to file streamer
         */
        int write_to_stream(const char * buf, uint64_t pos, uint64_t frame_size)
		{
			if (bInit)
			{
				bool bOK = true;

				//DXT_VERBOSE("Writing %d bytes to pos %d", frame_size, pos);

				// write operation
				if (is_good()) ofs->seekp(pos); else bOK = false;
				if (is_good()) ofs->write(buf, frame_size); else bOK = false;
				if (is_good()) ofs->flush(); else bOK = false;
				if (!is_good()) bOK = false;

				if (!bOK)
					HPV_ERROR("Error writing frame @ pos %d", pos);

				return bOK;
			}
			else
			{
				HPV_ERROR("Writing frame to non existing stream.");
				return -1;
			}
		}

		int write_header(const HPVHeader& header)
		{
			if (bInit)
			{
				int header_size = sizeof(uint32_t) * amount_header_fields;

				ofs->write((char *)&header, header_size);

				if (!is_good())
					return -1;

				ofs->flush();

				return 0;
			}
			else
			{
				HPV_ERROR("Writing header to non existing stream.");
				return -1;
			}

		}

		uint64_t get_current_pos()
		{
			return static_cast<uint64_t>(ofs->tellp());
		}

		void close()
		{
			if (ofs->is_open())
			{
				ofs->flush();
				ofs->close();
			}
			else
			{
				HPV_ERROR("Trying to close non existing stream");
			}
		}
	};
    
    
    class HPVCreator
	{
    public:
		HPVCreator(int version = HPV_VERSION_0_0_6);
        ~HPVCreator();
        int init(const HPVCreatorParams& params, ThreadSafe_Queue<HPVCompressionProgress> * progress_sink);
		int process_sequence(std::size_t amount_of_concurrency);
        void process_item(uint8_t thread_idx);
        void coordinate(uint8_t num_threads);
        void cancel();
        void reset();

    private:
        int version;
        std::string inpath;
        std::string outpath;
        int ref_width;
        int ref_height;
        uint32_t start_idx;
		uint32_t end_idx;
		uint8_t fps;
		HPVCompressionType type;

        std::unique_ptr<HPVFileStreamWriter> fs;

        uint32_t* frame_size_table;
        std::size_t bytes_per_frame;
        std::size_t bytes_in_header;
        std::size_t bytes_in_framesize_table;
        uint64_t offset_runner;

        uint64_t file_counter;

        std::vector<std::string> * file_names;

        std::atomic<bool> should_coordinate;
        std::thread coordinator_thread;

        std::vector<std::thread> work_threads;
        ThreadSafe_Queue<HPVCompressionWorkItem>   compression_queue;
		ThreadSafe_Map<HPVCompressedItem>          filestream_queue;

        uint32_t items_done_counter;
        ThreadSafe_Queue<HPVCompressionProgress> * progress_sink;
	};  
} /* namespace HPV */
  
#endif
