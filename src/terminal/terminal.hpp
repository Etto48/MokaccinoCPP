#pragma once
#include <string>
#include <vector>
#include <functional>
namespace terminal
{
    /**
     * @brief Used to specify the function relative to a command in the terminal
     * 
     */
    struct CommandFunction
    {
        /**
         * @brief command name, the first arg must equal this
         * 
         */
        std::string name;
        /**
         * @brief used to autogenerate the help command
         * 
         */
        std::string usage;
        /**
         * @brief used to autogenerate the help command
         * 
         */
        std::string help_text;
        /**
         * @brief funciton pointer to call when the command is entered, the first arg is always equal to name
         * 
         */
        bool (*function)(const std::string& line, const std::vector<std::string>& args) = nullptr;
        /**
         * @brief minimum argument number, if set to 0 is ignored
         * 
         */
        unsigned int min_argn = 0;
        /**
         * @brief maximum argument number, if set to 0 is ignored
         * 
         */
        unsigned int max_argn = 0;
        /**
         * @brief Construct a new Command Function struct
         * 
         * @param name command name, the first arg must equal this
         * @param usage used to autogenerate the help command
         * @param help_text used to autogenerate the help command
         * @param function funciton pointer to call when the command is entered, the first arg is always equal to name
         * @param min_argn minimum argument number, if set to 0 is ignored
         * @param max_argn maximum argument number, if set to 0 is ignored
         */
        CommandFunction(const std::string& name, const std::string& usage, const std::string& help_text, bool (*function)(const std::string& line, const std::vector<std::string>& args), unsigned int min_argn = 0, unsigned int max_argn = 0):
            name(name),
            usage(usage),
            help_text(help_text),
            function(function),
            min_argn(min_argn),
            max_argn(max_argn){};
        CommandFunction() = default;
    };
    /**
     * @brief add a command to terminal
     * 
     * @param command_info config and info about the command + the actual function to call
     */
    void add_command(const CommandFunction& command_info);
    /**
     * @brief parse and process a terminal command
     * 
     * @param line the input line to parse
     * @return true if the command succeeded
     * @return false if the command failed
     */
    bool process_command(const std::string& line);
    /**
     * @brief initialize the module
     * 
     */
    void init();
    /**
     * @brief request user input and run a callback function when it's received
     * 
     * @param prompt the prompt to show to the user
     * @param callback a function that will be called after the user presses enter
     */
    void input(const std::string& prompt, std::function<void(const std::string&)> callback);
    /**
     * @brief wait for input blocking the current thread
     * 
     * @param prompt the prompt to show to the user
     * @return the user input
     */
    std::string blocking_input(const std::string& prompt);
};