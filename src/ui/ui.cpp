#include "ui.hpp"
#include "../defines.hpp"
#include "../ansi_escape.hpp"
#include <mutex>
#include <iostream>
#include <list>
#ifndef NO_CURSES
#ifdef MOUSE_MOVED
#undef MOUSE_MOVED
#endif
#include <curses.h>
#endif
#include "../parsing/parsing.hpp"
namespace ui
{
    std::mutex interface_mutex;
    int lines_offset = 0;
    void scroll_text(int line_count);
    #ifdef NO_CURSES
    #define PRINT_EXCEPT_N_LINES 1
    std::list<std::string> lines; 
    std::pair<unsigned int, unsigned int> terminal_size;
    std::string getline()
    {
        std::string ret;
        std::getline(std::cin, ret);
        return ret;
    }
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
            std::cout << CLEAR_LINE;
        }
        restore_cursor();
    }
    void prompt(const std::string& str)
    {
        std::unique_lock lock(interface_mutex);
        save_cursor();
        move_cursor(terminal_size.first-1,1);
        auto spaces = terminal_size.second - parsing::strip_ansi(str).length() - 2;
        std::cout << CLEAR_LINE << PROMPT_COLOR << ' ' << str << std::string(spaces,' ') << RESET;
        restore_cursor();
    }
    void redraw_lines()
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
    void print(const std::string& str)
    {
        std::unique_lock lines_lock(interface_mutex);
        auto line = str;
        while(parsing::strip_ansi(line).length() > terminal_size.second)
        {
            auto pos = parsing::ansi_len(line,terminal_size.second);
            auto to_add = line.substr(0,pos);
            line = line.substr(pos);
            lines.push_front(to_add);
        }
        if(line.length() != 0)
        {
            lines.push_front(line);
        }
        redraw_lines();
    }
    void init()
    {
        terminal_size = get_terminal_size();
        move_cursor(terminal_size.first,1);
        clear();
    }
    void fini()
    {

    }
    #else
    WINDOW* scroller_window = nullptr;
    WINDOW* input_window = nullptr;
    std::list<std::vector<std::pair<char,int>>> lines;
    #ifndef _WIN32
    int mvwdeleteln(WINDOW* win, int line, int col)
    {
        for(int i = col; i<COLS; i++)
        {
            mvwdelch(win,line,i);
        }
        return 0;
    }
    #endif
    void print_colored_line(int line_number,std::vector<std::pair<char,int>> colored_line)
    {
        int iterator = 0;
        for(const auto& c: colored_line)
        {   
            wattron(scroller_window,COLOR_PAIR(c.second));
            mvwaddch(scroller_window,line_number,iterator,c.first);
            wattroff(scroller_window,COLOR_PAIR(c.second));
            iterator ++;
        }
    }
    void redraw_lines()
    {
        wclear(scroller_window);
        auto iterator = lines.begin();
        for(unsigned int i = 0;iterator!=lines.end() and i < (unsigned)lines_offset; i++)
            iterator++;
        for(int i = 0; i <= LINES - 2 - 1; i++)
        {
            if(iterator != lines.end())
            {
                print_colored_line(LINES - 2 - i - 1,*iterator);
                iterator++;
            }     
        }
        wrefresh(scroller_window);
    }
    void _scroll_text(int line_count);
    std::string getline_buffer;
    int getline_cursor = 0;
    std::string getline()
    {
        getline_buffer = "";
        std::string ret;
        int c;
        bool done = false;
        getline_cursor = 0;
        while(not done){
            c = getch();

            std::unique_lock lock(interface_mutex);
            if(c == -1)
                continue;
            if(c=='\n')
            {
                ret = getline_buffer;
                getline_buffer = "";
                getline_cursor = 0;
                break;
            }
            switch (c)
            {
            case KEY_BACKSPACE:
            case '\b':
                if(getline_cursor > 0)
                {
                    getline_buffer.erase(getline_cursor-1,1);
                    getline_cursor--;
                }
                break;
            case KEY_LEFT:
                if(getline_cursor > 0)
                    getline_cursor--;
                break;
            case KEY_RIGHT:
                if(getline_cursor < getline_buffer.length())
                    getline_cursor++;
                break;
            case KEY_HOME:
                getline_cursor = 0;
                break;
            case KEY_END:
                getline_cursor = (int)getline_buffer.length();
                break;
            case KEY_UP:
                _scroll_text(1);
                break;
            case KEY_DOWN:
                _scroll_text(-1);
                break;
            case KEY_SEND:
                lines_offset = 0;
                break;
            default:
                getline_buffer.insert(getline_buffer.begin()+getline_cursor,c);
                getline_cursor ++;
                break;
            }
            mvwaddstr(input_window,1,0,(getline_buffer + std::string(COLS - getline_buffer.length() + 1,' ')).c_str());
            move(LINES-1,getline_cursor);
            wrefresh(input_window);
        }
        lines_offset = 0;
        mvwaddstr(input_window,1,0,std::string(COLS,' ').c_str());
        move(LINES-1,0);
        wrefresh(input_window);
        return ret;
    }
    void prompt(const std::string& str)
    {
        std::unique_lock lock(interface_mutex);
        wattron(input_window,COLOR_PAIR(1));
        mvwdeleteln(input_window,0,0);
        mvwaddstr(input_window,0,0,(' '+str+std::string(COLS-str.length()-1,' ')).c_str());
        wattroff(input_window,COLOR_PAIR(1));
        mvwaddstr(input_window,1,0,getline_buffer.c_str());
        wmove(input_window,1,getline_cursor);
        wrefresh(input_window);
    }
    void print(const std::string& str)
    {
        std::unique_lock lines_lock(interface_mutex);
        auto line = parsing::curses_split_color(str);
        while(line.size() > COLS)
        {
            auto pos = COLS;
            auto to_add = std::vector(line.begin(),line.begin()+pos);
            line = std::vector(line.begin()+pos,line.end());
            lines.push_front(to_add);
        }
        if(line.size() != 0)
        {
            lines.push_front(line);
        }
        redraw_lines();
    }
    void init()
    {
        initscr();
        clear();
        if(has_colors())
        {
            start_color();
        }
        cbreak();
        noecho();
        intrflush(stdscr, FALSE);
        keypad(stdscr,true);
        
        scroller_window = newwin(LINES - 2, COLS, 0, 0);
        input_window = newwin(2,COLS,LINES-2,0);

        init_pair(1,COLOR_BLACK,COLOR_WHITE); // INVERTED
        init_pair(2,COLOR_MAGENTA,COLOR_BLACK); // HIGHLIGHT
        init_pair(3,COLOR_RED,COLOR_BLACK);// ERR_TAG      
        init_pair(4,COLOR_GREEN,COLOR_BLACK);// MESSAGE_PEER 
        init_pair(5,COLOR_YELLOW,COLOR_BLACK);// DBG_TAG      
        init_pair(6,COLOR_BLUE,COLOR_BLACK);// MSG_TAG      
        init_pair(7,COLOR_CYAN,COLOR_BLACK);// TAG          
        init_pair(8,COLOR_WHITE,COLOR_BLACK);// MESSAGE_TEXT

        wrefresh(scroller_window);
        wrefresh(input_window);
    }
    void fini()
    {
        delwin(scroller_window);
        delwin(input_window);
        endwin();
    }
    #endif
    void _scroll_text(int line_count)
    {
        lines_offset += line_count;
        if(lines_offset < 0)
            lines_offset = 0;
        else if(lines_offset > lines.size())
            lines_offset = (int)lines.size();
        
        if(line_count != 0)
        {
            redraw_lines();
        }
    }
    void scroll_text(int line_count)
    {
        std::unique_lock lines_lock(interface_mutex);
        _scroll_text(line_count);
    }

}