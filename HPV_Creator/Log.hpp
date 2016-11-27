/**********************************************************
* Holo_ToolSet
* http://github.com/HasseltVR/Holo_ToolSet
* http://www.uhasselt.be/edm
*
* Distributed under LGPL v2.1 Licence
* http ://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
*
* Parts are based on code from roxlu https://github.com/roxlu/roxlu_experimental
**********************************************************/
#ifndef HPV_LOG_H
#define HPV_LOG_H

#include <string>
#include <fstream>
#include <iostream>
#include <inttypes.h>


#define HPV_LOG_LEVEL_ERROR  1
#define HPV_LOG_LEVEL_WARNING 2
#define HPV_LOG_LEVEL_VERBOSE 3
#define HPV_LOG_LEVEL_DEBUG 4
#define HPV_LOG_LEVEL_ALL 5

#define HPV_LOG_TRUNCATE 0
#define HPV_LOG_APPEND 1

#if defined(_MSC_VER)
#  define HPV_DEBUG(fmt, ...) { hpv_debug(__LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); } 
#  define HPV_VERBOSE(fmt, ...) { hpv_verbose(__LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); } 
#  define HPV_WARNING(fmt, ...) { hpv_warning(__LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); } 
#  define HPV_ERROR(fmt, ...) { hpv_error(__LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); } 
#else                                                                             
#  define HPV_DEBUG(fmt, ...) { hpv_debug(__LINE__, __PRETTY_FUNCTION__, fmt, ##__VA_ARGS__); } 
#  define HPV_VERBOSE(fmt, ...) { hpv_verbose(__LINE__, __PRETTY_FUNCTION__, fmt, ##__VA_ARGS__); } 
#  define HPV_WARNING(fmt, ...) { hpv_warning(__LINE__, __PRETTY_FUNCTION__, fmt, ##__VA_ARGS__); } 
#  define HPV_ERROR(fmt, ...) { hpv_error(__LINE__, __PRETTY_FUNCTION__, fmt, ##__VA_ARGS__); } 
#endif

/* --------------------------------------------------------------------------------- */

namespace HPV {

  /* Debug control */
  int hpv_log_init(std::string path = "", int mode = HPV_LOG_APPEND);
  void hpv_log_disable_stdout();
  void hpv_log_enable_stdout();
  void hpv_log_disable_log_to_file();
  void hpv_log_enable_log_to_file();
  void hpv_log_set_level(int level);
  int hpv_log_get_level();

  /* Debug wrappers. */
  void hpv_debug(int line, const char* function, const char* fmt, ...);
  void hpv_verbose(int line, const char* function, const char* fmt, ...);
  void hpv_warning(int line, const char* function, const char* fmt, ...);
  void hpv_error(int line, const char* function, const char* fmt, ...);

  /* --------------------------------------------------------------------------------- */

  class Log 
  {
  public:
    Log();
    ~Log();
    int open(std::string filepath, int mode); /* Open the log with the give filepath. Mode 0 = append / 1: truncate */
    void log(int level,                    /* Log something at the given level. */
             int line,                     /* Refers to the line in the source code. */
             const char* function,         /* Funcion that logged the message. */
             const char* fmt,              /* We use printf() like formats. */ 
             va_list args);                /* Variable arguments. */ 

  public:
    bool write_to_stdout;                  /* Write output also to stdout. */
    bool write_to_file;                    /* Write log to file. */
    int level;                             /* What level we should log. */
                                           
  private:                                 
    std::string filepath;                  /* Filepath where we save the log file. */
    std::ofstream ofs;                     /* The output file stream */
  };

  /* --------------------------------------------------------------------------------- */

  extern Log hpv_log;

} /* namespace HPV */

#endif
