#pragma once
#include <tuple>
#include <string>
namespace ui
{
    std::pair<unsigned int, unsigned int> get_terminal_size();
    void move_cursor(unsigned int row = 1, unsigned int column = 1);
    void save_cursor();
    void restore_cursor();
    void clear();
    void clear_lines(unsigned int from, unsigned int to);

    bool get_page_up();
    bool get_page_down();

    void prompt(std::string prompt_text);
    void reprint_lines();
    void add_line(std::string line);
    void init();
}