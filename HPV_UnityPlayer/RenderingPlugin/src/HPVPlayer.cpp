#include "HPVPlayer.h"

namespace HPV {
    
	HPVPlayer::HPVPlayer()
	: _num_bytes_in_header(0)
	, _id(0)
    , _filesize(0)
    , _bytes_per_frame(0)
    , _new_frame_time(0)
    , _global_time_per_frame(0)
    , _local_time_per_frame(0)
    , _curr_frame(0)
    , _seeked_frame(0)
    , _loop_in(0)
    , _loop_out(0)
    , _mode(HPV_MODE_NORMAL)
    , _state(HPV_STATE_NONE)
    , _direction(HPV_DIRECTION_FORWARDS)
    , _frame_buffer(nullptr)
    , _is_init(false)
    , _gather_stats(false)
    , _m_event_sink(nullptr)
    , _frame_sizes_table(nullptr)
    {
        _should_update = false;
        _update_result.store(0, std::memory_order_relaxed);
        _was_seeked.store(false, std::memory_order_relaxed);
        _header.magic = 0;
        _header.version = 0;
        _header.video_width = 0;
        _header.video_height = 0;
        _header.number_of_frames = 0;
        _header.frame_rate = 0;
        _header.crc_frame_sizes = 0;
        _decode_stats.gpu_upload_time = 0;
        _decode_stats.hdd_read_time = 0;
        _decode_stats.l4z_decode_time = 0;
    }
    
    HPVPlayer::~HPVPlayer()
    {
        if (_is_init)
        {
            close();
        }
		HPV_VERBOSE("~HPVPLayer");
    }
    
    int HPVPlayer::open(const std::string& filepath)
    {
        if (true == _ifs.is_open())
        {
            HPV_ERROR("Already loaded %s, call shutdown if you want to reload.", filepath.c_str());
            return HPV_RET_ERROR;
        }
        
        if (0 == filepath.size())
        {
            HPV_ERROR("Invalid filepath; size is 0.");
            return HPV_RET_ERROR;
        }
        
        // open the input filestream
        _ifs.open(filepath.c_str(), std::ios::binary | std::ios::in);
        if (!_ifs.is_open())
        {
            HPV_ERROR("Failed to open: %s", filepath.c_str());
            return HPV_RET_ERROR;
        }
        
        // get filesize of HPV file
        _ifs.seekg(0, std::ifstream::end);
        _filesize = _ifs.tellg();
        _ifs.seekg(0, std::ios_base::beg);
        
        if (0 == _filesize)
        {
            _ifs.close();
            HPV_ERROR("File size is 0 (%s).", filepath.c_str());
            return HPV_RET_ERROR;
        }
        
        // read the header
        if (0 != HPV::readHeader(&_ifs, &_header))
        {
            HPV_ERROR("Failed to read HPV header from %s", filepath.c_str());
            return HPV_RET_ERROR;
        }
        
        if (_header.magic != HPV_MAGIC)
        {
            HPV_ERROR("Wrong magic number")
            return HPV_RET_ERROR;
        }
        
        // check if dimensions are in correct range
        if (0 == _header.video_width || _header.video_width > HPV_MAX_SIDE_SIZE)
        {
            HPV_ERROR("Video width is invalid. Either 0 or bigger than what we don't support yet. Video width: %u", _header.video_width);
            _ifs.close();
            return HPV_RET_ERROR;
        }
        
        if (0 == _header.video_height || _header.video_height > HPV_MAX_SIDE_SIZE)
        {
            HPV_ERROR("Video height is invalid. either 0 or bigger than what we don't support yet. Video height: %u", _header.video_height);
            _ifs.close();
            return HPV_RET_ERROR;
        }
        
        // good file, save its path
        _file_path = filepath;
        
        // ready reading the header...save our position
        _num_bytes_in_header = static_cast<uint32_t>(_ifs.tellg());
        _num_bytes_in_sizes_table = _header.number_of_frames * sizeof(uint32_t);
        
        // read in frame size table and check crc
        _frame_sizes_table = new uint32_t[_header.number_of_frames];
        _frame_offsets_table = new uint64_t[_header.number_of_frames];
        
        _ifs.read((char *)_frame_sizes_table, _num_bytes_in_sizes_table);
        
        uint32_t crc = 0;
        for (uint32_t i=0 ; i<_header.number_of_frames; ++i)
        {
            crc += _frame_sizes_table[i];
        }
        
        if (crc != _header.crc_frame_sizes)
        {
            HPV_ERROR("Frame sizes table CRC doesn't match, corrupt file")
            return HPV_RET_ERROR;
        }
        
        uint32_t start_offset = _num_bytes_in_header + _num_bytes_in_sizes_table;
        this->populateFrameOffsets(start_offset);
        
        // calculate frame size in bytes from compression type
        _bytes_per_frame = _header.video_width * _header.video_height;
        
        if (_header.compression_type == HPVCompressionType::HPV_TYPE_DXT1_NO_ALPHA)
        {
            _bytes_per_frame >>= 1;
        }
        else
        {
            //
        }
        
        // get the native frame rate of the file (was given as parameter during compression) and set initial speed to speed 1
        uint32_t fps = _header.frame_rate;
        _global_time_per_frame = static_cast<uint64_t>(double(1.0 / fps) * 1e9);
        
        HPV_VERBOSE("Loaded file '%s' [dims: %ux%u | fps: %u | frames: %u | type: %s | version: %u]",
                    filepath.substr(filepath.find_last_of("\\/")+1).c_str(),
                    _header.video_width,
                    _header.video_height,
                    _header.frame_rate,
                    _header.number_of_frames,
                    HPVCompressionTypeStrings[(uint8_t)_header.compression_type].c_str(),
                    _header.version
                    );
        
        // set initial state
        this->resetPlayer();
        
        // allocate the frame buffer, happens only once. We re-use the same memory space
        _frame_buffer = new unsigned char[_bytes_per_frame];
        
        if (!_frame_buffer)
        {
            HPV_ERROR("Failed to allocate the frame buffer.");
            _ifs.close();
            return HPV_RET_ERROR;
        }
        
        // read the first frame
        if (!readCurrentFrame())
        {
            HPV_ERROR("Failed to read the first frame.");
            _ifs.close();
            return HPV_RET_ERROR;
        }
        
        this->launchUpdateThread();
        
        _is_init = true;
        
        return HPV_RET_ERROR_NONE;
    }
    
    int HPVPlayer::close()
    {
        if (_is_init)
        {
            // first let thread finish
            _update_result.store(0, std::memory_order_release);
            _should_update = false;
            
            if (_update_thread.joinable())
            {
                _update_thread.join();
            }
            _update_result = 0;
            
            HPV_VERBOSE("Closed HPV worker thread for %s", _file_path.substr(_file_path.find_last_of("\\/")+1).c_str());
            
            if (_ifs.is_open())
            {
                _ifs.close();
            }
            
            if (_frame_buffer)
            {
                delete [] _frame_buffer;
                _frame_buffer = nullptr;
            }
            
            // clear out header
            memset(&_header, 0x00, HPV::amount_header_fields);
            
            _num_bytes_in_header = 0;
            _filesize = 0;
            _bytes_per_frame = 0;
            _new_frame_time = 0;
            _global_time_per_frame = 0;
            _local_time_per_frame = 0;
            
            _curr_frame = 0;
            _state = HPV_STATE_NONE;
            
            _is_init = false;
        }
        
        return HPV_RET_ERROR_NONE;
    }
    
    void HPVPlayer::populateFrameOffsets(uint32_t start_offset)
    {
        uint64_t offset_runner = (uint64_t)start_offset;
        _frame_offsets_table[0] = offset_runner;
        
        for (uint32_t frame_idx = 1; frame_idx < _header.number_of_frames; ++frame_idx)
        {
            offset_runner += _frame_sizes_table[frame_idx-1];
            _frame_offsets_table[frame_idx] = offset_runner;
        }
    }
    
    inline int HPVPlayer::readCurrentFrame()
    {
        static uint64_t _before_read, _before_decode;
        static uint64_t _after_read, _after_decode;
        
        if (_gather_stats)
        {
            _before_read = ns();
            _before_decode = _before_read;
        }
        
        _ifs.seekg(_frame_offsets_table[_curr_frame]);
        
        if (!_ifs.good())
        {
            HPV_ERROR("Failed to seek to %lu", _frame_offsets_table[_curr_frame]);
            return HPV_RET_ERROR;
        }
        
        // create local buffer for storing L4Z compressed frame
        char * _l4z_buffer = new char[ _frame_sizes_table[_curr_frame] ];
        
        // read L4Z data from disk into buffer
        _ifs.read(_l4z_buffer, _frame_sizes_table[_curr_frame]);
        
        if (_gather_stats)
        {
            _after_read = ns();
            
            _decode_stats.hdd_read_time = _after_read - _before_read;
        }
        
        if (!_ifs.good())
        {
            HPV_ERROR("Failed to read frame %" PRId64, _curr_frame);
            return HPV_RET_ERROR;
        }
        
        if (_gather_stats)
        {
            _before_decode = ns();
        }
        
        // decompress L4Z
        int ret_decomp = LZ4_decompress_fast((const char *)_l4z_buffer, (char *)_frame_buffer, static_cast<int>(_bytes_per_frame));
        
        if (ret_decomp == 0)
        {
            HPV_ERROR("Failed to decompress frame %" PRId64, _curr_frame);
            return HPV_RET_ERROR;
        }
        
        if (_gather_stats)
        {
            _after_decode = ns();
            
            _decode_stats.l4z_decode_time = _after_decode - _before_decode;
        }
        
        delete [] _l4z_buffer;
        
        _update_result.store(1, std::memory_order_relaxed);
        
        return HPV_RET_ERROR_NONE;
    }
    
    void HPVPlayer::launchUpdateThread()
    {
        // start thread now that everything is set for this player
        _should_update = true;
        _update_thread = std::thread(&HPVPlayer::update, this);
    }
    
    int HPVPlayer::play()
    {
        if (!_ifs.is_open())
        {
            HPV_ERROR("Trying to play, but the file stream is not opened. Did you call init()?");
            return HPV_RET_ERROR;
        }
        
        if (isPlaying())
        {
            HPV_ERROR("Already playing.");
            return HPV_RET_ERROR;
        }
        
        _new_frame_time = ns() + _local_time_per_frame;
        
        _state = HPV_STATE_PLAYING;
        
        notifyHPVEvent(HPVEventType::HPV_EVENT_PLAY);
        
        return HPV_RET_ERROR_NONE;
    }
    
    int HPVPlayer::play(int fps)
    {
        if (!_ifs.is_open())
        {
            HPV_ERROR("Trying to play, but the file stream is not opened. Did you call init()?");
            return HPV_RET_ERROR;
        }
        
        if (isPlaying())
        {
            HPV_ERROR("Already playing.");
            return HPV_RET_ERROR;
        }
        
        _global_time_per_frame = static_cast<uint64_t>(double(1.0 / fps) * 1e9);
        _local_time_per_frame = _global_time_per_frame;
        _new_frame_time = ns() + _local_time_per_frame;
        
        _state = HPV_STATE_PLAYING;
        
        notifyHPVEvent(HPVEventType::HPV_EVENT_PLAY);
        
        return HPV_RET_ERROR_NONE;
    }
    
    int HPVPlayer::pause()
    {
        if (!isPlaying())
        {
            HPV_ERROR("Cannot pause because we're not playing.");
            return HPV_RET_ERROR;
        }
        
        if (isPaused())
        {
            HPV_VERBOSE("Calling pause() on a video that's already paused.");
            return HPV_RET_ERROR;
        }
        
        _state = HPV_STATE_PAUSED;
        
        notifyHPVEvent(HPVEventType::HPV_EVENT_PAUSE);
        
        return HPV_RET_ERROR_NONE;
    }
    
    int HPVPlayer::resume()
    {
        if (!isPaused())
        {
            HPV_VERBOSE("Calling resume() on a video that's not paused.");
            return HPV_RET_ERROR;
        }
        
        _new_frame_time = ns() + _local_time_per_frame;
        
        
        _state = HPV_STATE_PLAYING;
        
        notifyHPVEvent(HPVEventType::HPV_EVENT_RESUME);
        
        return HPV_RET_ERROR_NONE;
    }
    
    int HPVPlayer::stop()
    {
        if (!isPlaying() && !isPaused())
        {
            HPV_ERROR("Cannot stop because we're not paying or paused.");
            return HPV_RET_ERROR;
        }
        
        _state = HPV_STATE_STOPPED;
        
        _curr_frame = _loop_in;
        readCurrentFrame();
        
        notifyHPVEvent(HPVEventType::HPV_EVENT_STOP);
        
        return HPV_RET_ERROR_NONE;
    }
    
    int HPVPlayer::setLoopMode(uint8_t loop_mode)
    {
        _mode = loop_mode;
        
        return HPV_RET_ERROR_NONE;
    }
    
    int HPVPlayer::setLoopInPoint(int64_t loop_in)
    {
        if (loop_in >= 0 && loop_in < _header.number_of_frames)
        {
            _loop_in = loop_in;
            
            return HPV_RET_ERROR_NONE;
        }
        
        return HPV_RET_ERROR;
    }
    
    int HPVPlayer::setLoopOutPoint(int64_t loop_out)
    {
        if (loop_out > _loop_in && loop_out < _header.number_of_frames)
        {
            _loop_out = loop_out;
            seek(_loop_in);
            
            return HPV_RET_ERROR_NONE;
        }
        
        return HPV_RET_ERROR;
    }
    
    /*
     *	Threaded function. Updates the times until a new frame should be read.
     *
     *	The result is stored in std::atomic<bool> updateResult. This way, the main thread
     *	can query when a new frame is ready.
     *	updateResult > 0	-> new frame is ready
     *	updateResult = 0	-> not yet there, still iterating
     *
     */
    void HPVPlayer::update()
    {
        while (_should_update)
        {
            uint64_t now;
            
            if (_was_seeked)
            {
                _curr_frame = _seeked_frame;
                _was_seeked.store(false, std::memory_order_relaxed);
                now = ns();
            }
            else
            {
                /* When not playing, paused or stopped: don't do anything. */
                if (!isPlaying() || isPaused() || isStopped())
                {
                    continue;
                }
                
                /* When playing: get delta time and check if we need to load next frame. */
                now = ns();
                
                if (!(now >= _new_frame_time))
                {
                    continue;
                }
                // next actions depening on playback direction: forwards / backwards
                else if (HPV_DIRECTION_FORWARDS == _direction)
                {
                    ++_curr_frame;
                    
                    if (_curr_frame >= _loop_out)
                    {
                        notifyHPVEvent(HPVEventType::HPV_EVENT_LOOP);
                        if (HPV_MODE_NORMAL == _mode)
                        {
                            stop();
                        }
                        else if (HPV_MODE_LOOP == _mode)
                        {
                            _curr_frame = _loop_in;
                            
                        }
                        else if (HPV_MODE_PALINDROME == _mode)
                        {
                            _curr_frame = _loop_out;
                            _direction = HPV_DIRECTION_REVERSE;
                        }
                        else
                        {
                            HPV_ERROR("Unhandled play mode.");
                            continue;
                        }
                    }
                }
                else
                {
                    --_curr_frame;
                    
                    if (_curr_frame <= _loop_in)
                    {
                        notifyHPVEvent(HPVEventType::HPV_EVENT_LOOP);
                        if (HPV_MODE_NORMAL == _mode)
                        {
                            stop();
                            
                        }
                        else if (HPV_MODE_LOOP == _mode)
                        {
                            _curr_frame = _loop_out;
                        }
                        else if (HPV_MODE_PALINDROME == _mode)
                        {
                            _curr_frame = _loop_in;
                            _direction = HPV_DIRECTION_FORWARDS;
                        }
                        else
                        {
                            HPV_ERROR("Unhandled play mode.");
                            continue;
                        }
                    }
                }
            }
            
            /* Set future time when new frame is needed */
            _new_frame_time = now + _local_time_per_frame;
            
            /* Read the frame from the file */
            if (!readCurrentFrame())
            {
                continue;
            }
            
            // sleep thread and wake up in time for next frame, taking in account read time
            std::this_thread::sleep_for(std::chrono::nanoseconds(_local_time_per_frame-(_decode_stats.hdd_read_time+_decode_stats.l4z_decode_time)-1000000));
        }
    }
    
    int HPVPlayer::setSpeed(double speed)
    {
        // don't take in account speeds too close to 0!
        if (speed < HPV_SPEED_EPSILON && speed > -HPV_SPEED_EPSILON)
        {
            // avoid stopping the video!
			return HPV_RET_ERROR;
        }
        
        // fwd
        if (speed > HPV_SPEED_EPSILON)
        {
            _direction = HPV_DIRECTION_FORWARDS;
        }
        // bwd
        else if (speed < -HPV_SPEED_EPSILON)
        {
            _direction = HPV_DIRECTION_REVERSE;
        }
        
        // the frame duration of course changes when the speed changes
        _local_time_per_frame = static_cast<uint64_t>(_global_time_per_frame / std::abs(speed));
        
        // resume if we were paused earlier (speed +/- 0)
        if (isPaused())
        {
            resume();
        }
        
        return HPV_RET_ERROR_NONE;
    }
    
    int HPVPlayer::setPlayDirection(uint8_t direction)
    {
        if (direction)
        {
			_direction = HPV_DIRECTION_FORWARDS;
        }
        else
        {
            _direction = HPV_DIRECTION_REVERSE;
        }
        
        return HPV_RET_ERROR_NONE;
    }
    
    int HPVPlayer::seek(double pos)
    {
        if (pos < 0.0 || pos > 1.0)
            return HPV_RET_ERROR;
        
        if (HPV::isNearlyEqual(pos, 0.0))
        {
            _seeked_frame = 0;
        }
        else if (HPV::isNearlyEqual(pos, 1.0))
        {
            _seeked_frame = _header.number_of_frames - 1;
        }
        else
        {
            _seeked_frame = static_cast<int64_t>(std::floor(_header.number_of_frames * pos));
        }
        
        _was_seeked.store(true, std::memory_order_relaxed);
        
        return HPV_RET_ERROR_NONE;
    }
    
    int HPVPlayer::seek(int64_t frame)
    {
        if (frame < 0 || frame >= _header.number_of_frames)
            return HPV_RET_ERROR;
        
        _seeked_frame = frame;
        
        _was_seeked.store(true, std::memory_order_relaxed);
        
        return HPV_RET_ERROR_NONE;
    }
    
    void HPVPlayer::resetPlayer()
    {
        _local_time_per_frame = _global_time_per_frame;
        _loop_in = 0;
        _loop_out = _header.number_of_frames - 1;
        _direction = HPV_DIRECTION_FORWARDS;
    }
    
    int HPVPlayer::isLoaded()
    {
        return _is_init;
    }
    
    int HPVPlayer::isPlaying()
    {
		return (_state == HPV_STATE_PLAYING);
    }
    
    int HPVPlayer::isPaused()
    {
		return (_state == HPV_STATE_PAUSED);
    }
    
    int HPVPlayer::isStopped()
    {
        return (_state == HPV_STATE_STOPPED);
    }
    
    void HPVPlayer::notifyHPVEvent(HPVEventType type)
    {
        if (_m_event_sink)
        {
            HPVEvent event(type, this);
            _m_event_sink->push(event);
        }
    }
    
    int HPVPlayer::getWidth()
    {
        return _header.video_width;
    }
    
    int HPVPlayer::getHeight()
    {
        return _header.video_height;
    }
    
    std::size_t HPVPlayer::getBytesPerFrame()
    {
        return _bytes_per_frame;
    }
    
    unsigned char* HPVPlayer::getBufferPtr()
    {
        return _frame_buffer;
    }
    
    int HPVPlayer::getFrameRate()
    {
        return _header.frame_rate;
    }
    
    HPVCompressionType HPVPlayer::getCompressionType()
    {
        return _header.compression_type;
    }
    
    std::string HPVPlayer::getFilePath()
    {
        return _file_path;
    }
    
    int64_t HPVPlayer::getCurrentFrameNumber()
    {
        return _curr_frame;
    }
    
    uint64_t HPVPlayer::getNumberOfFrames()
    {
        return _header.number_of_frames;
    }
    
    float HPVPlayer::getPosition()
    {
        if (_header.number_of_frames <= 0)
        {
            return 0;
        }
        
        return _curr_frame / static_cast<float>(_header.number_of_frames);
    }
    
    bool HPVPlayer::hasNewFrame()
    {
        // only reset new frame flag when renderer has displayed the new frame
        if (_update_result)
        {
            _update_result = 0;
            return true;
        }
        else
            return false;
    }
    
    int HPVPlayer::enableStats(bool _enable)
    {
        _gather_stats = _enable;
        
        return HPV_RET_ERROR_NONE;
    }
    
    void HPVPlayer::addHPVEventSink(ThreadSafe_Queue<HPVEvent> * sink)
    {
        _m_event_sink = sink;
    }
    
} /* namespace HPV */
