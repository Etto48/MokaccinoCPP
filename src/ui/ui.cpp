#include "ui.hpp"
#include "../defines.hpp"
#include "../ansi_escape.hpp"
#include "../parsing/parsing.hpp"
#include "../multithreading/multithreading.hpp"
#include <boost/thread.hpp>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#endif
#include <iostream>
#include <list>
#include <mutex>
#include <semaphore>
namespace ui
{
    std::pair<unsigned int, unsigned int> terminal_size;
    std::mutex lines_mutex;
    std::list<std::string> lines; 
    #define PRINT_EXCEPT_N_LINES 1  

    #ifdef _WIN32
    std::pair<unsigned int, unsigned int> get_terminal_size()
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        unsigned int columns, rows;

        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        return {rows,columns};
    }
    #else
    std::pair<unsigned int, unsigned int> get_terminal_size()
    {
        winsize w;
        ioctl(0, TIOCGWINSZ, &w);

        return {w.ws_row,w.ws_col};
    }
    #endif
    void move_cursor(unsigned int row, unsigned int column)
    {
        std::cout << "\033[" << std::to_string(row) << ';' << std::to_string(column) << 'H';
        std::cout.flush();
    }
    void save_cursor()
    {
        std::cout << SAVE_CURSOR;
        std::cout.flush();
    }
    void restore_cursor()
    {
        std::cout << RESTORE_CURSOR;
        std::cout.flush();
    }
    void clear()
    {
        std::cout << CLEAR_SCREEN;
        std::cout.flush();
    }
    void clear_lines(unsigned int from, unsigned int to)
    {
        save_cursor();
        for(unsigned int i = from; i<to; i++)
        {
            move_cursor(1,i);
            std::cout << "\033[K";
        }
        restore_cursor();
    }

    #ifdef _WIN32
    bool get_page_up()
    {
        return (GetAsyncKeyState(VK_F2) & 0x8000);
    }
    bool get_page_down()
    {
        return (GetAsyncKeyState(VK_F3) & 0x8000);
    }
    #else
    bool get_page_up()
    {
        return false;
    }
    bool get_page_down()
    {
        return false;
    }
    #endif

    void prompt(std::string prompt_text)
    {
        save_cursor();
        move_cursor(terminal_size.first-1,1);
        auto spaces = terminal_size.second - parsing::strip_ansi(prompt_text).length() - 2;
        std::cout << CLEAR_LINE << PROMPT_COLOR << ' ' << prompt_text << std::string(spaces,' ') << RESET;
        restore_cursor();
    }

    int lines_offset = 0;
    void _reprint_lines()
    {
        clear_lines(1,terminal_size.first-PRINT_EXCEPT_N_LINES);
        save_cursor();
        auto iterator = lines.begin();
        for(unsigned int i = 0;iterator!=lines.end() and i < (unsigned)lines_offset; i++)
            iterator++;
        for(unsigned int i = 0; i < terminal_size.first - PRINT_EXCEPT_N_LINES - 1; i++)
        {
            move_cursor(terminal_size.first - PRINT_EXCEPT_N_LINES - i - 1,1);
            std::cout << CLEAR_LINE;
            if(iterator != lines.end())
            {
                std::cout << *iterator;
                iterator++;
            }     
        }
        restore_cursor();
    }
    void reprint_lines()
    {
        std::unique_lock lines_lock(lines_mutex);
        _reprint_lines();
    }    

    void add_line(std::string line)
    {
        std::unique_lock lines_lock(lines_mutex);
        while(parsing::strip_ansi(line).length() > terminal_size.second or line.find('\n') != line.npos)
        {
            auto where_newline = parsing::strip_ansi(line).find('\n');
            if(where_newline != line.npos and where_newline < terminal_size.second)
            {// break line on \n
                auto pos = parsing::ansi_len(line,where_newline);
                auto to_add = line.substr(0,pos);
                line = line.substr(pos+1);
                lines.push_front(to_add);
            }
            else
            {// break line on length
                auto pos = parsing::ansi_len(line,terminal_size.second);
                auto to_add = line.substr(0,pos);
                line = line.substr(pos);
                lines.push_front(to_add);
            }
        }
        if(line.length() != 0)
        {
            lines.push_front(line);
        }
    }

    void input_handler()
    {
        #ifndef NO_TERMINAL_UI
        while(true)
        {
            auto page_down = get_page_down();
            auto page_up = get_page_up();
            int move = page_down and not page_up? -1 : page_up and not page_down? 1 : 0;
            
            {
                std::unique_lock lines_lock(lines_mutex);
                lines_offset += move;
                if(lines_offset < 0)
                    lines_offset = 0;
                else if(lines_offset > lines.size())
                    lines_offset = (int)lines.size();
                
                if(move != 0)
                {
                    _reprint_lines();
                }
            }
            boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
        }
        #endif
    }
    void init()
    {
        terminal_size = get_terminal_size();
        move_cursor(terminal_size.first,1);
        multithreading::add_service("input_handler",input_handler);
        clear();
    }
}
