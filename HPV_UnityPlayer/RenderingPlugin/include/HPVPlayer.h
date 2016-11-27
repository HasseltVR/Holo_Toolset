/**********************************************************
* Holo_ToolSet
* http://github.com/HasseltVR/Holo_ToolSet
* http://www.uhasselt.be/edm
*
* Distributed under LGPL v2.1 Licence
* http ://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
**********************************************************/
#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <stdint.h>
#include <thread>
#include <atomic>
#include <cmath>
#include <memory>

#include "Log.h"
#include "HPVHeader.h"
#include "HPVEvent.h"
#include "ThreadSafeQueue.h"
#include "Timer.h"

#define HPV_READ_PATH_ERROR			0x00
#define HPV_READ_HEADER_ERROR		0x01
#define HPV_READ_DIMENSION_ERROR	0x02

#define HPV_STATE_NONE				0x00
#define HPV_STATE_PLAYING			0x01
#define HPV_STATE_PAUSED			0x02
#define HPV_STATE_STOPPED			0x04

#define HPV_MODE_NORMAL				0x00    /* Plays once, then stops. */
#define HPV_MODE_LOOP				0x01    /* Will continuously play, when the end frame has been reached we switch back tot he first one. */
#define HPV_MODE_PALINDROME			0x02    /* Will loop back and forth */

#define HPV_DIRECTION_FORWARDS		0x00
#define HPV_DIRECTION_REVERSE		0x01

#define HPV_SPEED_EPSILON			0.05

/* --------------------------------------------------------------------------------- */
namespace HPV {
    
    // see Knuth section 4.2.2 pages 217-218
    inline bool isNearlyEqual(double x, double y)
    {
        const double epsilon = 1e-5;
        return std::abs(x - y) <= epsilon * std::abs(x);
    }
    
    typedef struct
    {
        uint64_t hdd_read_time;
        uint64_t l4z_decode_time;
        uint64_t gpu_upload_time;
    } HPVDecodeStats;
    
    class HPVPlayer
    {
    public:
        HPVPlayer();
        ~HPVPlayer();
        int             open(const std::string& filepath);
        int             play();
        int             play(int fps);
        int             pause();
        int             resume();
        int             stop();
        int             close();
        
        int             setLoopMode(uint8_t loop_mode);
        int             setLoopInPoint(int64_t loop_in);
        int             setLoopOutPoint(int64_t loop_out);
        int             setPlayDirection(uint8_t direction);
        int             setSpeed(double speed);
        int             seek(double pos);
        int             seek(int64_t frame);
        
        int             getWidth();
        int             getHeight();
        std::size_t     getBytesPerFrame();
        unsigned char*  getBufferPtr();
        int64_t         getCurrentFrameNumber();
        uint64_t        getNumberOfFrames();
        
        void            addHPVEventSink(ThreadSafe_Queue<HPVEvent> * sink);
        void            notifyHPVEvent(HPVEventType type);
        
        void            launchUpdateThread();
        void            update();
        bool            hasNewFrame();
        void            resetPlayer();
        
        float           getPosition();
        int             getFrameRate();
        HPVCompressionType getCompressionType();
        
        std::string     getFilePath();
        
        int             isLoaded();
        int             isPlaying();
        int             isPaused();
        int             isStopped();
        
		uint8_t			_id;
        bool            _gather_stats;
        HPVDecodeStats  _decode_stats;
        int             enableStats(bool get_stats);
        
    private:
        HPVHeader       _header;
       
        std::ifstream   _ifs;
        std::string     _file_path;
        uint32_t        _num_bytes_in_header;
        uint32_t        _num_bytes_in_sizes_table;
        size_t          _filesize;
        uint32_t *      _frame_sizes_table;
        uint64_t *      _frame_offsets_table;
        size_t          _bytes_per_frame;
        uint64_t        _new_frame_time;
        uint64_t        _global_time_per_frame;
        uint64_t        _local_time_per_frame;
        int64_t         _curr_frame;
        int64_t         _seeked_frame;
        int64_t         _loop_in;
        int64_t         _loop_out;
        uint8_t         _mode;
        volatile bool   _should_update;
        std::atomic<int> _update_result;
        std::atomic<bool> _was_seeked;
        std::thread     _update_thread;
        int             _state;
        int             _direction;
        bool            _is_init;
        
        unsigned char*  _frame_buffer;
        
        void            populateFrameOffsets(uint32_t);
        int             readCurrentFrame();
        
        ThreadSafe_Queue<HPVEvent> * _m_event_sink;
    };
    
    typedef std::shared_ptr<HPV::HPVPlayer> HPVPlayerRef;
    
} /* namespace HPV */
