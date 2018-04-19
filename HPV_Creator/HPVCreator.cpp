// These have to be defined in order for the DXT and IMAGE libraries to load their functionality
#define STB_DXT_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include <sys/stat.h>

#include "stb_dxt.h"
#include "stb_image.h"
#include "HPVCreator.hpp"

namespace HPV {

    static HPVCompressionProgress error;
    
    bool file_supported(const std::string& path)
    {
        if (path.empty())
            return false;
        
        const size_t pos = path.find_last_of(".");
        
        if (pos == std::string::npos || pos == 0)
        {
            return false;
        }
        
        const std::string ext = path.substr(pos+1, std::string::npos);
        
        return (std::find(HPV::supported_filetypes.begin(),
                          HPV::supported_filetypes.end(), ext)
                != HPV::supported_filetypes.end());
    }

    HPVCreator::HPVCreator(int _version) : version(_version)
	{
        frame_size_table = nullptr;
        progress_sink = nullptr;
        file_names = nullptr;
        should_coordinate.store(false, std::memory_order_relaxed);
	}

	HPVCreator::~HPVCreator()
	{

	}

    int HPVCreator::init(const HPVCreatorParams& _params, ThreadSafe_Queue<HPVCompressionProgress> * _progress_sink)
	{
        if (file_counter > 0)
        {
            reset();
        }

        error.state = HPV_CREATOR_STATE_ERROR;

        // this is where post progress messages
        if (_progress_sink)
        {
            progress_sink = _progress_sink;
        }
        else
        {
            return HPV_RET_ERROR;
        }

        if (_params.in_path.size() == 0) {
            error.done_item_name = std::string("Given input path is invalid, size is 0.");
            progress_sink->push(error);
            return HPV_RET_ERROR;
		}

        if (_params.out_path.size() == 0)
		{
            error.done_item_name = std::string("Given output path is invalid, size is 0.");
            progress_sink->push(error);
            return HPV_RET_ERROR;
		}

        if (_params.out_frame < _params.in_frame) {
            error.done_item_name = std::string("Invalid start or end frame");
            progress_sink->push(error);
            return HPV_RET_ERROR;
		}

		if (0 >= _params.fps)
		{
            error.done_item_name = std::string("Frame rate cannot be <= 0!");
            progress_sink->push(error);
            return HPV_RET_ERROR;
		}

		if (_params.type >= HPVCompressionType::HPV_NUM_TYPES)
		{
            error.done_item_name = std::string("Unrecognised compression type!");
            progress_sink->push(error);
            return HPV_RET_ERROR;
		}

        if (_params.file_names == nullptr)
        {
            error.done_item_name = std::string("File list is null!");
            progress_sink->push(error);
            return HPV_RET_ERROR;
        }

		// store params
		this->inpath = _params.in_path;
		this->outpath = _params.out_path;
		this->start_idx = _params.in_frame;
		this->end_idx = _params.out_frame;
		this->fps = _params.fps;
		this->type = _params.type;
        this->file_names = _params.file_names;

        // We need to load the first image to get it's dimenions. This will serve as a reference
		// meaning that all other images will need to be the exact same size. 
		// If not, we will skip that image and try to load the next
		int w = 0;
		int h = 0;
		int ch = 0;
        std::string name_str = file_names->at(0);

		// check existence of file
		struct stat buffer;
        if (stat(name_str.c_str(), &buffer) != 0)
		{
            error.done_item_name = "Couldn't reference first file in directory " + name_str;
            progress_sink->push(error);
            return HPV_RET_ERROR;
		}

		// load the file, always load as RGBA
        unsigned char * pixels = stbi_load(name_str.c_str(), &w, &h, &ch, 4);

		if (!pixels)
		{
            error.done_item_name = "Couldn't load pixels for file";
            progress_sink->push(error);
            return HPV_RET_ERROR;
        }

		if (w == 0 || w > HPV_MAX_SIDE_SIZE)
		{
            error.done_item_name = "File has invalid width";
            progress_sink->push(error);
            return HPV_RET_ERROR;
        }

		if (h == 0 || h > HPV_MAX_SIDE_SIZE)
		{
            error.done_item_name = "File has invalid height";
            progress_sink->push(error);
            return HPV_RET_ERROR;
        }

		if ( (w % 4 != 0) || (h % 4 != 0))
		{
            error.done_item_name = "Input images are not a power of two or less than 4x4.";
            progress_sink->push(error);
            return HPV_RET_ERROR;
        }

        // store reference frame sizes, all frames must be same size
		ref_width = w;
		ref_height = h;

        // the reference byte size (uncompressed) per frame is depending on the compression type
        bytes_per_frame = ref_width*ref_height;

		if (type == HPVCompressionType::HPV_TYPE_DXT1_NO_ALPHA)
		{
			if (ch == 4) HPV_VERBOSE("DXT1 without alpha selected as compression type on source with alpha, bypassing alpha");
            bytes_per_frame /= 2;
		}
		else if (type == HPVCompressionType::HPV_TYPE_DXT5_ALPHA && ch == 3)
		{
			HPV_VERBOSE("DXT5 with alpha selected for source without alpha. Using DXT1 compression instead.")
			type = HPVCompressionType::HPV_TYPE_DXT1_NO_ALPHA;
            bytes_per_frame /= 2;
		}
		else if (type == HPVCompressionType::HPV_TYPE_SCALED_DXT5_CoCg_Y)
		{
			//
		}

        HPV_VERBOSE("Reference dimensions are %dx%d, type %s yielding %d bytes per frame", ref_width, ref_height, HPVCompressionTypeStrings[(int)type].c_str(), bytes_per_frame);

		// save first file for later processing
        HPVCompressionWorkItem item;
        
        item.path = name_str;
        item.offset = 0;
        compression_queue.push(item);
        
        ++file_counter;

		// go over all the remaining files to check if they are 'good'.
		// If so, store path in vector to process later. 
        for (uint32_t i = start_idx+1; i <= end_idx; ++i)
		{
            name_str = file_names->at(i);

			// check existence of file
			struct stat buffer;
            if (stat(name_str.c_str(), &buffer) != 0)
			{
                error.done_item_name = "Couldn't load file" + name_str + ": skipping";
                progress_sink->push(error);
				continue;
			}

			// Valid file, store for later processing. 
			// During processing we just check the dimensions against the reference dimensions.
            item.path = name_str;
            item.offset = file_counter;
            compression_queue.push(item);

            ++file_counter;
		}
        HPV_VERBOSE("Compression queue now has %d items to process.", compression_queue.size());

		// open the file stream writes which will take care of all filewrite operations 
		fs = std::make_unique<HPVFileStreamWriter>();

		if (!fs->init(outpath))
		{
			HPV_ERROR("Error while opening output path %s", outpath.c_str());
		}

		// fill DXT header struct
		HPVHeader header;
        header.magic = HPV_MAGIC;
		header.version = this->version;
		header.video_width = ref_width;
		header.video_height = ref_height;
		header.number_of_frames = 0;	// will fill in later, after all valid frames were processed
		header.frame_rate = fps;
		header.compression_type = type;
        header.crc_frame_sizes = 0;
        header.reserved_1 = 0;
        header.reserved_2 = 0;

		// write the header
		fs->write_header(header);

		// store how many bytes are in header
		bytes_in_header = fs->get_current_pos();

        // initialize frame size table
        frame_size_table = new (std::nothrow) uint32_t[compression_queue.size()];
        if (!frame_size_table)
        {
            error.done_item_name = "Error allocating memory";
            progress_sink->push(error);
        }

        for (std::size_t i = 0; i < compression_queue.size(); ++i)
        {
			frame_size_table[i] = 0;
        }

        // write empty
        bytes_in_framesize_table = compression_queue.size() * sizeof(uint32_t);
        fs->write_to_stream( (const char *)frame_size_table, bytes_in_header, bytes_in_framesize_table);

        // save current offset to start writing frame data later
        offset_runner = bytes_in_header + bytes_in_framesize_table;

        return HPV_RET_ERROR_NONE;
	}

    void HPVCreator::coordinate(uint8_t num_threads)
    {
        uint32_t length = static_cast<uint32_t>(compression_queue.size());

        uint64_t start = ns();

        // initialize threads
        for (uint8_t i = 0; i < num_threads; ++i)
        {
            work_threads.push_back(std::thread(&HPVCreator::process_item, this, i));
        }

        // each increment of our counter results in a key to look up a new valid compressed frame
        items_done_counter = 0;
        offset_runner = bytes_in_header + bytes_in_framesize_table;
        uint32_t crc = 0;

        while (should_coordinate.load())
        {
            // Try to fetch item with next key from queue and wait if it's not yet in queue.
            std::shared_ptr<HPVCompressedItem> item = filestream_queue.wait_and_pop(items_done_counter);

            if (item)
            {
                // Writing is key successive, so the writer doesn't have to seek to
                // non-neighbouring frame positions
                item->write_pos = offset_runner;

                fs->write_to_stream(item);

                if (!fs->is_good())
                {
                    error.done_item_name = "Error writing to disk for " + item->path;
                    progress_sink->push(error);
                    break;
                }

                // free the out buffer once it's written to disk
                free(item->write_out_buf);

                ++items_done_counter;

                offset_runner += item->frame_size;
                crc += static_cast<uint32_t>(item->frame_size);

                HPVCompressionProgress progress;
                progress.state = HPV_CREATOR_STATE_BUSY;
                progress.total_items = length;
                progress.done_items = items_done_counter;
                progress.done_item_name = item->path;
                progress.compression_ratio = item->compression_ratio;
                progress_sink->push(progress);
            }

            // wrote all DXT frames to disk
            if (items_done_counter == length && compression_queue.empty())
            {
                should_coordinate.store(false, std::memory_order_relaxed);
            }
        }

        // join all threads
        std::for_each(work_threads.begin(), work_threads.end(), std::mem_fn(&std::thread::join));
        
        uint64_t end = ns();

        uint64_t compressed_total_size = 0;

        for (uint32_t i = 0; i < items_done_counter; ++i)
        {
            compressed_total_size += frame_size_table[i];
        }

        // rewrite our successful frames in the header
        fs->write_to_stream((const char *)&length, 16, 4);

        // in the end: rewrite crc of frame sizes table
        fs->write_to_stream((const char *)&crc, 28, 4);

        // rewrite our frame sizes table
        fs->write_to_stream((const char *)frame_size_table, bytes_in_header, bytes_in_framesize_table);

        // close the file stream
        fs->close();

        HPVCompressionProgress done;
        std::stringstream ss;
        ss  << "Done converting images to "
            << outpath
            << std::endl
            << "Converting took: "
            << (end-start) / 1e9
            << " seconds"
            << std::endl
            << "Final size (LZ4 HQ level 9) is: "
            << compressed_total_size / 1e9
            << " GB";

        done.state = HPV_CREATOR_STATE_DONE;
        done.done_item_name = ss.str();
        progress_sink->push(done);
    }

    void HPVCreator::process_item(uint8_t thread_idx)
    {
        // wait thread_idx * 100ms before starting, to avoid all threads rushing to
        // start fetching items from compression queue
        std::this_thread::sleep_for(std::chrono::milliseconds(thread_idx * 100));

        // if our output stream doesn't exist, quit
        if (!fs->is_good())
        {
            error.done_item_name = "No filestream writer!";
            progress_sink->push(error);
            return;
        }

        // otherwise:
        // - do some checking on the file
        // - load pixels
        // - compress pixels to corresponding BC format
        // - write to thread-safe FileStreamWriter
        int w = 0;
        int h = 0;
        int channels = 0;

        while (!compression_queue.empty())
        {
            // wait a bit when file stream writer already has a lot of work
            if (filestream_queue.size() > PROCESSED_SIZE_BARRIER)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(PROCESSED_BARRIER_SLEEPTIME));
            }

            // try to load one item from the compression queue
            std::shared_ptr<HPVCompressionWorkItem> item = compression_queue.try_pop();

            if (item)
            {
                unsigned char* pixels = nullptr;
                std::size_t compressed_size = 0;

                unsigned char* dxt = new(std::nothrow) unsigned char[bytes_per_frame];
                if (!dxt)
                {
                    error.done_item_name = "Failed to allocate the texture compressed buffer.";
                    progress_sink->push(error);
                    break;
                }

                std::size_t write_buf_size = LZ4_COMPRESSBOUND(bytes_per_frame);
                char* write_buf = new(std::nothrow) char[write_buf_size];
                if (!write_buf)
                {
                    error.done_item_name = "Failed to allocate the L4Z compressed write buffer.";
                    progress_sink->push(error);
                    break;
                }

                // load the file, always load as RGBA
                pixels = stbi_load(item->path.c_str(), &w, &h, &channels, 4);

                if (!pixels)
                {
                    error.done_item_name = "Failed to load pixel data from " + item->path;
                    progress_sink->push(error);
                    delete [] dxt;
                    delete [] write_buf;
                    break;
                }

                if (w != ref_width && h != ref_height)
                {
                    std::stringstream ss;
                    ss << "File "
                       << item->path
                       << " has incorrect dimensions: "
                       << w
                       << "x"
                       << h
                       << " Should be: "
                       << ref_width
                       << "x"
                       << ref_height
                       << std::endl
                       << "Signaling stop";
                    error.done_item_name = ss.str();
                    progress_sink->push(error);
                    stbi_image_free(pixels);
                    delete [] dxt;
                    delete [] write_buf;
                    break;
                }

                // We are ready to compress our input pixels. Different methods apply:
                //
                // - RGB pixels can be compressed as
                //		* DXT1:			[RGB input]:	ok image quality, no alpha, 0.5 bpp
                //		* scaled DXT5:	[CoCg_Y input]:	good image quality, no alpha, 1bpp
                //
                // - RGBA pixels can be compressed as:
                //		* DXT5:			[RGBA input]:	ok image quality, alpha with good gradients, 1bpp
                if (HPVCompressionType::HPV_TYPE_DXT1_NO_ALPHA == type)
                {
                    rygCompress(dxt, pixels, w, h, false);
                }
                else if (HPVCompressionType::HPV_TYPE_DXT5_ALPHA == type)
                {
                    rygCompress(dxt, pixels, w, h, true);
                }
                else if (HPVCompressionType::HPV_TYPE_SCALED_DXT5_CoCg_Y == type)
                {
                    // YCoCg conversion if DXT5
                    ConvertRGBToCoCg_Y(pixels, w, h);

                    // DXT compress
                    CompressYCoCgDXT5(pixels, dxt, w, h, w * 4);
                }

                // compress resulting DXT buffer more with LZ4
                compressed_size = LZ4_compress_HC((const char *)dxt, write_buf, static_cast<int>(bytes_per_frame), static_cast<int>(bytes_per_frame), HPV_LZ4_COMPRESSION_LEVEL);

                if (compressed_size == 0)
                {
                    HPV_VERBOSE("Error LZ4 compressing DXT frame");
                    continue;
                }

                // write compressed size to frame size index table
                frame_size_table[item->offset] = static_cast<uint32_t>(compressed_size);

                HPVCompressedItem compressed_item;
                compressed_item.write_out_buf       = write_buf;
                compressed_item.frame_size          = compressed_size;
                compressed_item.path                = item->path;
                compressed_item.compression_ratio   = (compressed_size / (float)bytes_per_frame) * 100.f;
                filestream_queue.push(compressed_item, item->offset);

                // clear pixels and dxt buffers for next image, write buffer will be freed by writer
                stbi_image_free(pixels);
                delete [] dxt;
            }
        }
    }

	int HPVCreator::process_sequence(std::size_t amount_of_concurrency)
	{
        if (!compression_queue.size())
            return HPV_RET_ERROR;

		unsigned int const max_threads = static_cast<unsigned int const>(amount_of_concurrency);
		unsigned int const hardware_threads = std::thread::hardware_concurrency();
		unsigned int const num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
		HPV_VERBOSE("Dividing work over %d threads", num_threads);

        should_coordinate.store(true, std::memory_order_relaxed);
        coordinator_thread = std::make_unique<std::thread>(&HPVCreator::coordinate, this, num_threads);

        return HPV_RET_ERROR_NONE;
	}

    void HPVCreator::stop()
    {
        // must obey specific order!
        should_coordinate.store(false, std::memory_order_relaxed);

        compression_queue.clear();

        if (coordinator_thread && coordinator_thread->joinable())
        {
            coordinator_thread->join();
            coordinator_thread.reset();
        }
    }

    void HPVCreator::reset()
    {
        if (should_coordinate.load())
        {
            stop();
        }
        else
        {
            if (compression_queue.size()) compression_queue.clear();
            if (work_threads.size()) work_threads.clear();
            if (filestream_queue.size()) filestream_queue.clear();
        }

        offset_runner = 0;
        file_counter = 0;

        if (frame_size_table)
        {
            delete[] frame_size_table;
            frame_size_table = nullptr;
        }
        
        inpath = "";
        outpath = "";
        ref_width = 0;
        ref_height = 0;
        start_idx = 0;
        end_idx = 0;
        fps = 0;
        type = HPVCompressionType::HPV_NUM_TYPES;
        fs = nullptr;
        bytes_per_frame = 0;
        bytes_in_header = 0;
        bytes_in_framesize_table = 0;
        offset_runner = 0;
        file_names = nullptr;
        file_counter = 0;
    }
} /* namespace HPV */
