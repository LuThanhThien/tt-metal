#ifndef TTSIM_H
#define TTSIM_H

#include <iostream>

#define HOST "HOST" 
#define READER "READER"
#define WRITER "WRITER"
#define COMPUTE "COMPUTE"

#define ENDL std::endl
#define PRINT_SEP std::cout << "--------------------------------------------------------" << std::endl
#define SLASH "////////////////////////////////////////////////////////\n"
#define PRINT_LINE std::cout << "\n" << std::endl

#define PRINT_TITLE(title) std::cout << SLASH << "//\t\t" << title << "\n" << SLASH << std::endl
#define PRINT_HOST std::cout << "[" << HOST << "] "
#define PRINT_CORE(processor_type, core_id) std::cout << "[" << processor_type << " CORE " << core_id << "] "
#define PRINT_LEVEL(level) for(int i = 0; i < (level); ++i) std::cout << "\t"

#endif // TTSIM_H

