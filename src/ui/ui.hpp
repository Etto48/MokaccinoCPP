#pragma once
#include <string>
#include <exception>
namespace ui
{
    /**
     * @brief scroll the screen of "line_count" lines
     * 
     * @param line_count this is the number of lines, positive for upwards, negative for downwards
     */
    void scroll_text(int line_count);
    class KeyboardInterrupt: public std::exception
    {
    public:
        const char* what(){return "KeyboardInterrupt";}
    };
    /**
     * @brief blocking get line function, returns a string when the users hits enter, while the function is running 
     * even the not printable keys will be handled, if CTRL-C is hit throws KeyboardInterrupt
     * 
     * @return a string inputted from the user
     */
    std::string getline();
    /**
     * @brief change the prompt shown in the input section
     * 
     * @param str the new prompt
     */
    void prompt(const std::string& str);
    /**
     * @brief print a line without '\n'
     * 
     * @param str the line to print
     */
    void print(const std::string& str);
    void print(const std::string& prefix, const std::string& msg);
    /**
     * @brief initialize the module
     * 
     */
    void init();
    /**
     * @brief release the resources initialized during init
     * 
     */
    void fini();
}