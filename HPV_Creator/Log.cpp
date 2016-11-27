#include "Log.hpp"
#include <sstream>
#if defined(_WIN32)
# include <stdarg.h>
# include <time.h>
#elif defined(__linux)
# include <stdarg.h>
#else
# include <sys/time.h>
#endif

#if defined(__APPLE__)
#  include <TargetConditionals.h>
#endif

namespace HPV {

  Log hpv_log;

  /* --------------------------------------------------------------------------------- */

  /* @todo - cleanup the if()s around the ofs file writes. */
  Log::Log() 
    :write_to_stdout(true)
    ,write_to_file(true)
    ,level(HPV_LOG_LEVEL_ALL)
  {
  }

  Log::~Log() {

    if (ofs.is_open()) {
      ofs.close();
    }

    write_to_stdout = false;
    write_to_file = false;
  }

  int Log::open(std::string filep, int mode) {
    
    if (false == write_to_file) {
      return 0;
    }

    if (0 != filepath.size()) {
      printf("Error: trying to open the log file but it's already open? Calling hpv_log_init() twice?\n");
      return -1;
    }

    filepath = filep;

    if (0 == filepath.size()) {
      printf("Error: cannot open the log filepath because the string is empty.\n");
      return -2;
    }

    if (HPV_LOG_APPEND == mode)
		ofs.open(filepath.c_str(), std::ios::out | std::ios::app);
	else if (HPV_LOG_TRUNCATE == mode)
		ofs.open(filepath.c_str(), std::ios::out | std::ios::trunc);

    if (!ofs.is_open()) {
      printf("Error: cannot open the log file. No permission? %s\n", filepath.c_str());
      return -3;
    }

    return 0;
  }

  void Log::log(int inlevel, int line, const char* function, const char* fmt, va_list args) {
  
    if (inlevel > level) {
      return;
    }

    static char buffer[1024 * 8]; /* should be big enough ;-) */
    std::string slevel;
    std::stringstream ss;
    std::stringstream ss_stdout;
    std::string color_msg_open;
    std::string color_info_open;
    std::string color_close;

    if (true == write_to_file) {
      if (false == ofs.is_open()) {
		//d  printf("Error: cannot log because the file hasn't been opened. Did you call dxt_log_init()?\n");
       // printf("Error: cannot log because the file hasn't been opened. Did you call hpv_log_init()?\n");
        return;
      }
    }

    /* default colors. */
#if defined(_WIN32) || TARGET_OS_IPHONE
    color_close = "";
    color_info_open = "";
#else
    color_close = "\e[0m";
    color_info_open = "\e[90m";
#endif

    vsprintf(buffer, fmt, args);

    if (write_to_file) {
        time_t ltime; /* calendar time */
        ltime=time(NULL); /* get current cal time */
        ofs << asctime( localtime(&ltime)) << " " ;
    }


    if (inlevel == HPV_LOG_LEVEL_DEBUG) {
      slevel = " debug ";
#if !defined(_WIN32) && !TARGET_OS_IPHONE
      color_msg_open = "\e[36m";
#endif
      if (write_to_file) {
        ofs << slevel;
      }
    }
    else if (inlevel == HPV_LOG_LEVEL_VERBOSE) {
      slevel = " verbose ";

      if (write_to_file) {
        ofs << slevel;
      }

#if !defined(_WIN32) && !TARGET_OS_IPHONE
      color_msg_open = "\e[92m";
#endif
    }
    else if (inlevel == HPV_LOG_LEVEL_WARNING) {
      slevel =  " warning ";
#if !defined(_WIN32) && !TARGET_OS_IPHONE
      color_msg_open = "\e[93m";
#endif
      
      if (write_to_file) {
        ofs << slevel;
      }
    }
    else if (inlevel == HPV_LOG_LEVEL_ERROR) {
      slevel = " <<ERROR>> ";
#if !defined(_WIN32) && !TARGET_OS_IPHONE
      color_msg_open = "\e[31m";
#endif
      if (write_to_file) {
        ofs << slevel;
      }
    }

    if (write_to_file) {
      ofs << " [" << function << ":" << line << "] = " <<  buffer << "\n";
    }

    if (write_to_stdout) {
      ss << time(NULL) << ":"
         << slevel 
         << "[" << function << ":" << line << "]"
         << " = " << buffer
         << std::endl;

      ss_stdout << color_info_open 
                << time(NULL) << ":" 
                << slevel 
                << "[" << function << ":" << line << "]"
                << " = " 
                << color_msg_open
                << buffer
                << color_close
                << std::endl;
      printf("%s", ss_stdout.str().c_str());
    }

    if (write_to_file) {
      ofs.flush();
    }
  }

  /* --------------------------------------------------------------------------------- */

  int hpv_log_init(std::string path, int mode) {
    std::stringstream ss;
    std::string filepath = "";
    char buf[4096];
    time_t t;
    struct tm* info;

    time(&t);
    info = localtime(&t);
    strftime(buf, 4096, "%Y.%m.%d", info);

    if (0 == path.size()) {
      filepath = "./";
    }
    else {
      filepath = path +"/";
    }

    ss << filepath << "log-" << buf << ".log";
    filepath = ss.str();

    return hpv_log.open(filepath, mode);
  }

  void hpv_log_disable_stdout() {
    hpv_log.write_to_stdout = false;
  }

  void hpv_log_enable_stdout() {
    hpv_log.write_to_stdout = false;
  }

  void hpv_log_disable_log_to_file() {
    hpv_log.write_to_file = false;
  }

  void hpv_log_enable_log_to_file() {
    hpv_log.write_to_file = true;
  }

  void hpv_log_set_level(int level) {
    hpv_log.level = level;
  }

  int hpv_log_get_level() {
    return hpv_log.level;
  }

  void hpv_debug(int line, const char* function, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    hpv_log.log(HPV_LOG_LEVEL_DEBUG, line, function, fmt, args);
    va_end(args);
  }

  void hpv_verbose(int line, const char* function, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    hpv_log.log(HPV_LOG_LEVEL_VERBOSE, line, function, fmt, args);
    va_end(args);
  }

  void hpv_warning(int line, const char* function, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    hpv_log.log(HPV_LOG_LEVEL_WARNING, line, function, fmt, args);
    va_end(args);
  }

  void hpv_error(int line, const char* function, const char* fmt, ...) {
    //std::cout << fmt;
    va_list args;
    va_start(args, fmt);
    hpv_log.log(HPV_LOG_LEVEL_ERROR, line, function, fmt, args);
    va_end(args);
  }

} /* namespace HPV */
