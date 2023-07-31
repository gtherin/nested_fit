// Brief  : Module for parsing latex math input (does not support all the features).
//          This is a very simple 'parser' that uses regex
// Author : César Godinho
// Date   : 26/07/2023

// Supported features :
// - Params need to be 1 letter (+1 optional subscript letter)
// - Function calls need to be under \mathrm{func_name}(other_func(x)) or \texttt{func_name}(other_func(x))
// - Supported built-ins : \frac, \sqrt, \exp, \log, \sin, \cos, \tan
// - Supported contants  : \pi
// - Superscript is not allowed (only for powers)

// Lets keep C++11 compliant
#include <memory>
#include <regex>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <cassert>
#include <fstream>

typedef char ShortVarName[2];
typedef char LongVarName[64];

extern "C" struct Parameter
{
    LongVarName  name;       // The name (This really is only size 3 here)
    ShortVarName identifier; // The identifier (not null terminated)
};

extern "C" struct ParseOutput
{
    // ShortVarName* constants;     // The detected constants in the expression (Automatically converted for now)
    Parameter*    parameters;    // The detected parameters in the expression
    int           num_params;    // The number of parameters
    LongVarName*  functions;     // The detected function calls in the expression
    int           num_funcs;     // The number of function calls
    int*          func_argc;     // The detected number of parameters for each called user function in 'functions'
    char*         infixcode_f90; // The latex code converted to f90 for compilation
    int           error;         // The error code of the last error
};

#define LTXP_ERR_NOERR              0
#define LTXP_ERR_UNKOWN_PARAM       1
#define LTXP_ERR_INVALID_PARAMCOUNT 2
#define LTXP_ERR_INVALID_FUNCTION   3

#define ERR_MAP_ENTRY(x) {x, #x}
static const std::map<int, std::string> error_map = {
    ERR_MAP_ENTRY(LTXP_ERR_NOERR),
    ERR_MAP_ENTRY(LTXP_ERR_UNKOWN_PARAM),
    ERR_MAP_ENTRY(LTXP_ERR_INVALID_PARAMCOUNT),
    ERR_MAP_ENTRY(LTXP_ERR_INVALID_FUNCTION)

};

static std::vector<std::pair<std::string, std::string>> FindArgs(
    const char ref1[3],
    const char ref2[3],
    const std::string& expression,
    const std::ptrdiff_t offset_start,
    std::string& input,
    std::function<std::string(const std::string&, const std::string&)> replace_template
)
{
    std::vector<std::pair<std::string, std::string>> args;
    std::smatch match;
    std::string::const_iterator searchStart(input.cbegin());
    std::vector<std::pair<std::ptrdiff_t, std::size_t>> replace;
    std::vector<std::string> replace_what;
    while(std::regex_search(searchStart, input.cend(), match, std::regex(expression)))
    {
        int ref_count = 1;
        std::string::const_iterator it = searchStart + match.position(0) + offset_start;
        while(ref_count > 0)
        {
            if(*++it == ref1[0]) ref_count++;
            else if(*it == ref1[1]) ref_count--;
        }

        std::string arg1(searchStart + match.position(0) + offset_start, it);
        // std::cout << arg1 << std::endl;

        ref_count = 1;
        std::string::const_iterator it_den_start = ++++it;
        while(ref_count > 0)
        {
            if(*++it == ref2[0]) ref_count++;
            else if(*it == ref2[1]) ref_count--;
        }

        std::string arg2(it_den_start, it++);
        // std::cout << arg2 << std::endl;
        args.push_back(std::make_pair(arg1, arg2));

        replace.push_back(std::make_pair(match.position(0) + (searchStart - input.cbegin()), it - (searchStart + match.position(0))));
        replace_what.push_back(replace_template(arg1, arg2));
        searchStart = match.suffix().first;
    }

    std::ptrdiff_t offset = 0;
    for(size_t i = 0; i < replace.size(); i++)
    {
        input.replace(replace[i].first + offset, replace[i].second, replace_what[i]);
        offset += (replace_what[i].size() - replace[i].second);
    }

    return args;
}

static std::vector<std::string> FindArg(
    const char ref[3],
    const std::string& expression,
    const std::ptrdiff_t offset_start,
    std::string& input,
    std::function<std::string(const std::string&)> replace_template
)
{
    std::vector<std::string> args;
    std::smatch match;
    std::string::const_iterator searchStart(input.cbegin());
    std::vector<std::pair<std::ptrdiff_t, std::size_t>> replace;
    std::vector<std::string> replace_what;
    while(std::regex_search(searchStart, input.cend(), match, std::regex(expression)))
    {
        int ref_count = 1;
        std::string::const_iterator it = searchStart + match.position(0) + offset_start;
        while(ref_count > 0)
        {
            if(*++it == ref[0]) ref_count++;
            else if(*it == ref[1]) ref_count--;
        }

        std::string arg1(searchStart + match.position(0) + offset_start, it++);
        // std::cout << arg1 << std::endl;
        args.push_back(arg1);

        replace.push_back(std::make_pair(match.position(0) + (searchStart - input.cbegin()), it - (searchStart + match.position(0))));
        replace_what.push_back(replace_template(arg1));
        searchStart = match.suffix().first;
    }

    std::ptrdiff_t offset = 0;
    for(size_t i = 0; i < replace.size(); i++)
    {
        input.replace(replace[i].first + offset, replace[i].second, replace_what[i]);
        offset += (replace_what[i].size() - replace[i].second);
    }

    return args;
}

static void ReplaceToken(const std::string& token, const std::string& replacement, std::string& input)
{
    std::smatch match;
    std::string::const_iterator searchStart(input.cbegin());
    std::vector<std::pair<std::ptrdiff_t, std::size_t>> replace;
    std::vector<std::string> replace_what;
    while(std::regex_search(searchStart, input.cend(), match, std::regex(token)))
    {
        replace.push_back(std::make_pair(match.position(0) + (searchStart - input.cbegin()), match.str().length()));
        replace_what.push_back(replacement);
        searchStart = match.suffix().first;
    }

    std::ptrdiff_t offset = 0;
    for(size_t i = 0; i < replace.size(); i++)
    {
        input.replace(replace[i].first + offset, replace[i].second, replace_what[i]);
        offset += (replace_what[i].size() - replace[i].second);
    }
}

static void InsertAfterToken(
    const std::string& token,
    const std::string& what,
    std::string& input,
    std::function<bool(const std::ptrdiff_t&, const std::string&)> condition
)
{
    std::smatch match;
    std::string::const_iterator searchStart(input.cbegin());
    std::vector<std::ptrdiff_t> position;
    std::vector<std::string> insert_what;
    while(std::regex_search(searchStart, input.cend(), match, std::regex(token)))
    {
        position.push_back(match.position(0) + (searchStart - input.cbegin()) + match.str().length());
        insert_what.push_back(what);
        searchStart = match.suffix().first;
    }

    std::ptrdiff_t offset = 0;
    for(size_t i = 0; i < position.size(); i++)
    {
        if(condition(position[i] + offset, input))
        {
            input.insert(position[i] + offset, insert_what[i]);
            offset += insert_what[i].size();
        }
    }
}

static std::string RemoveAll(const std::string& input, const std::string& kw)
{
    std::string output;
    output.reserve(input.length());
    std::string::size_type lastPos = 0;
    std::string::size_type findPos;

    while(std::string::npos != (findPos = input.find(kw, lastPos)))
    {
        output.append(input, lastPos, findPos - lastPos);
        // output += to;
        lastPos = findPos + kw.length();
    }

    // Care for the rest after last occurrence
    output += input.substr(lastPos);

    return output;
}

static std::string StripInput(const std::string& input, const std::vector<std::string>& keywords)
{
    std::string output = input;
    
    for(const auto& kw : keywords)
    {
        output = RemoveAll(output, kw);
    }

    return output;
}

static void AddMultiplicationSignsReduceVars(std::string& input, const std::vector<std::pair<std::string, std::string>>& user_funcs)
{
    if(user_funcs.size() > ('Z' - 'A' + 1))
    {
        std::cout << "Parsing : For now, only " << ('Z' - 'A' + 1) << " function calls are allowed per expression" << std::endl;
    }

    // Replace user defined function names on this stage just to ignore the rules on them
    std::string dummy_name("USERFCN");
    char id = 'A';
    for(const auto& f : user_funcs)
    {
        ReplaceToken(f.first, dummy_name + id++, input);
    }

    // Rule for closing braces
    InsertAfterToken("\\)", "*", input, [](const std::ptrdiff_t& i, const std::string& input) -> bool {
        return input[i] != ')' &&
               input[i] != '+' &&
               input[i] != '-' &&
               input[i] != '*' &&
               input[i] != '/' &&
               input[i] != ',' &&
               i != input.length();
    });

    // Rule for digits/numbers
    InsertAfterToken("\\d", "*", input, [](const std::ptrdiff_t& i, const std::string& input) -> bool {
        return input[i] != ')' &&
               input[i] != '+' &&
               input[i] != '-' &&
               input[i] != '*' &&
               input[i] != '/' &&
               input[i] != ',' &&
               input[i] != '.' &&
               (input[i]  < '0' || input[i]  > '9') && i != input.length();
    });

    // Rule for variable x
    InsertAfterToken("x", "*", input, [](const std::ptrdiff_t& i, const std::string& input) -> bool {
        return input[i] != ')' &&
               input[i] != '+' &&
               input[i] != '-' &&
               input[i] != '*' &&
               input[i] != '/' &&
               input[i] != ',' &&
               input[i] != '_' &&
               i != input.length();
    });

    // Rule for identifiers
    InsertAfterToken("\\w_.", "*", input, [](const std::ptrdiff_t& i, const std::string& input) -> bool {
        return input[i] != ')' &&
               input[i] != '+' &&
               input[i] != '-' &&
               input[i] != '*' &&
               input[i] != '/' &&
               input[i] != ',' &&
               input[i] != '_' &&
               i != input.length();
    });

    // Rule for constants (\pi)
    InsertAfterToken("\\\\pi", "*", input, [](const std::ptrdiff_t& i, const std::string& input) -> bool {
        return input[i] != ')' &&
               input[i] != '+' &&
               input[i] != '-' &&
               input[i] != '*' &&
               input[i] != '/' &&
               input[i] != ',' &&
               i != input.length();
    });

    // Reduce identifier names before putting back function names to avoid collision
    input = RemoveAll(input, "_");

    // Put back the user defined functions with their names
    id = 'A';
    for(const auto& f : user_funcs)
    {
        ReplaceToken(dummy_name + id++, f.first, input);
    }
}

static std::vector<std::string> ExtractParameters(const std::string& stripped_input)
{
    std::vector<std::string> parameters;
    std::smatch match;
    std::string::const_iterator searchStart(stripped_input.cbegin());
    while(std::regex_search(searchStart, stripped_input.cend(), match, std::regex("\\w_.")))
    {
        parameters.push_back(match.str());
        searchStart = match.suffix().first;
    }

    std::string new_input = StripInput(stripped_input, parameters);

    // Strip x or digit or spaces or digit.digit
    searchStart = new_input.cbegin();
    while(std::regex_search(searchStart, new_input.cend(), match, std::regex("(?!x|\\d| |\\.|,).{1}")))
    {
        parameters.push_back(match.str());
        searchStart = match.suffix().first;
    }

    return parameters;
}

static std::vector<std::string> StringSplit(const std::string& input, const char delimiter)
{
    std::vector<std::string> output;
    std::stringstream ss(input);
    std::string token;

    while(std::getline(ss, token, delimiter))
    {
        output.push_back(token);
    }

    return output;
}

static std::string TrimLR(const std::string& input)
{
    std::string output(input);
    output.erase(output.begin(), std::find_if(output.begin(), output.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    output.erase(std::find_if(output.rbegin(), output.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), output.end());
    return output;
}

static int ArgumentCount(const std::string& input)
{
    if(TrimLR(input).size() == 0) return 0;

    int argc = 1;
    for(int i = 0; i < static_cast<int>(input.size()); i++)
    {
        if(input[i] == '(')
        {
            while(input[++i] != ')');
        }
        if(input[i] == ',') argc++;
    }
    return argc;
}

static std::tuple<std::string, std::vector<std::string>, std::vector<std::string>, std::vector<int>> SimplifyInput(const char* input_stream, bool* error)
{
    if(error != nullptr) *error = false; 
    const std::string start_string(input_stream);
    std::string input(input_stream);
    std::smatch match;

    // Fractions
    (void)FindArgs("{}", "{}", "\\\\frac\\{", 6, input, [](const std::string& a1, const std::string& a2) { return "((" + a1 + ")/(" + a2 + "))"; });

    // User functions
    auto functions = FindArgs("{}", "()", "\\\\texttt\\{|mathrm\\{", 8, input, [](const std::string& a1, const std::string& a2) { return a1 + "(" + a2 + ")"; });

    // Built-ins
    const std::map<std::string, std::string> f90_builtins = {
        { "sqrt", "DSQRT" },
        { "exp",   "DEXP" },
        { "log",   "DLOG" },
        { "sin",   "DSIN" },
        { "cos",   "DCOS" },
        { "tan",   "DTAN" }
    };

    std::vector<std::string> built_ins;
    for(const auto& function : f90_builtins)
    {
        // Match '(' start
        auto match0 = FindArg("()", "\\\\" + function.first + "\\(", function.first.length() + 2, input, [=](const std::string& a) { return function.second + "(" + a + ")"; });
        if(match0.size() > 0 ) built_ins.insert(built_ins.end(), match0.begin(), match0.end());
        
        // Match '{' start
        auto match1 = FindArg("{}", "\\\\" + function.first + "\\{", function.first.length() + 2, input, [=](const std::string& a) { return function.second + "(" + a + ")"; });
        if(match1.size() > 0 ) built_ins.insert(built_ins.end(), match1.begin(), match1.end());
    }

    // Replace ^ by **
    ReplaceToken("\\^", "**", input);

    std::vector<std::string> keywords;
    for(const auto& b : f90_builtins)
    {
        keywords.push_back(b.second);
    }
    for(const auto& f : functions)
    {
        keywords.push_back(f.first);
    }
    keywords.push_back("\\pi");
    keywords.push_back("(");
    keywords.push_back(")");
    keywords.push_back("/");
    keywords.push_back("+");
    keywords.push_back("-");
    keywords.push_back("*");

    auto parameters = ExtractParameters(StripInput(input, keywords));

    // Remove spaces
    input = RemoveAll(input, " ");

    // Place multiplication signs (*) in the final string
    AddMultiplicationSignsReduceVars(input, functions);

    // Replace \pi by pi
    ReplaceToken("\\\\pi", "pi", input);

    if(auto idx = input.find("\\") != std::string::npos)
    {
        std::cout << "Error at: " << input.substr(idx+1, 2) <<  ". Unrecognized parameter." << std::endl;
        if(error != nullptr) *error = true; 
    }

    if((error != nullptr && !*error) || error == nullptr)
    {
        std::cout << "========= Parsing result =========" << std::endl;

        std::cout << "     Latex input: ";
        std::cout << start_string << std::endl;

        std::cout << "Infix expression: ";
        std::cout << input << std::endl;
        std::cout << "      Parameters: ";
        for(size_t i = 0; i < parameters.size() - 1; i++)
        {
            std::cout << parameters[i] << ", ";
        }
        std::cout << parameters[parameters.size() - 1] << std::endl;
        std::cout << "==================================" << std::endl;
    }

    // Pass the function identifiers and arg count
    std::vector<std::string> function_names;
    std::vector<int> function_argcount;
    function_names.reserve(functions.size());
    for(const auto& f : functions)
    {
        function_names.push_back(f.first);
        function_argcount.push_back(ArgumentCount(f.second));
    }

    return std::make_tuple(input, parameters, function_names, function_argcount);
}

extern "C" ParseOutput ParseLatexToF90(const char* input_stream)
{
    ParseOutput return_val;
    return_val.error = LTXP_ERR_NOERR;
    bool error;

    auto output = SimplifyInput(input_stream, &error);
    if(error)
    {
        return_val.error = LTXP_ERR_UNKOWN_PARAM;
    }

    const std::string infix_code              = std::get<0>(output);
    const std::vector<std::string> parameters = std::get<1>(output); // Includes duplicates
    const std::vector<std::string> functions  = std::get<2>(output); // Includes duplicates
    const std::vector<int>         call_argc  = std::get<3>(output); // Includes duplicates

    assert(call_argc.size() == functions.size());

    return_val.infixcode_f90 = (char*)malloc(sizeof(char) * (infix_code.size() + 1));
    strcpy(return_val.infixcode_f90, infix_code.c_str());

    return_val.functions   = (LongVarName*)malloc(sizeof(LongVarName) * functions.size());
    return_val.num_funcs   = static_cast<int>(functions.size());
    return_val.func_argc = (int*)malloc(sizeof(int) * functions.size());
    for(size_t i = 0; i < functions.size(); i++)
    {
        assert(functions[i].size() < 64);
        strcpy(return_val.functions[i], functions[i].c_str());
        return_val.func_argc[i] = call_argc[i];
    }

    return_val.parameters = (Parameter*)malloc(sizeof(Parameter) * parameters.size());
    return_val.num_params = static_cast<int>(parameters.size());
    for(size_t i = 0; i < parameters.size(); i++)
    {
        assert(parameters[i].size() <= 3);
        strcpy(return_val.parameters[i].name, parameters[i].c_str());
        std::string p = RemoveAll(parameters[i], "_").c_str();
        return_val.parameters[i].identifier[0] = p[0];
        return_val.parameters[i].identifier[1] = p[1];
    }

    return return_val;
}

extern "C" void FreeParseOutput(ParseOutput* output)
{
    if(output != nullptr)
    {
        if(output->parameters)    free(output->parameters);
        // if(output->constants)     free(output->constants);
        if(output->functions)     free(output->functions);
        if(output->func_argc)     free(output->func_argc);
        if(output->infixcode_f90) free(output->infixcode_f90);
    }
}

extern "C" int CheckError(ParseOutput* output)
{
    if(output->error != LTXP_ERR_NOERR)
    {
        return 1;
    }
    return 0;
}

extern "C" void GetErrorMsg(ParseOutput* output, char* buffer)
{
    if(output->error)
    {
        strcpy(buffer, error_map.at(output->error).c_str());
    }
}

/*
* Cache specification:
* funcname - num_params - date
*/
extern "C" void CheckParseValidity(ParseOutput* output, const char* function_cache_path)
{
    std::ifstream input(function_cache_path);
    std::string line;

    std::vector<std::string> names;
    std::vector<int> varcount;

    int errorc = 0;
    while(std::getline(input, line))
    {
        auto        cache_line = StringSplit(line, '-');
        std::string name       = TrimLR(cache_line[0]);
        int         num_vars   = std::atoi(cache_line[1].c_str());
        std::string date_mod   = TrimLR(cache_line[2]);

        names.push_back(name);
        varcount.push_back(num_vars);
    }

    for(int i = 0; i < output->num_funcs; i++)
    {
        bool func_in_cache = false;
        for(int j = 0; j < static_cast<int>(names.size()); j++)
        {
            if(strcmp(output->functions[i], names[j].c_str()) == 0)
            {
                func_in_cache = true;
                if(output->func_argc[i] != varcount[j])
                {
                    std::cout << "Error on function call `" << output->functions[i] <<  "`: Function takes " << varcount[j] << " parameters, but " << output->func_argc[i] << " were specified." << std::endl;
                    output->error = LTXP_ERR_INVALID_PARAMCOUNT;
                }
            }
        }

        if(!func_in_cache)
        {
            std::cout << "Error on function call `" << output->functions[i] <<  "`: Function is not declared in nested_fit cache." << std::endl;
            output->error = LTXP_ERR_INVALID_FUNCTION;
        }
    }
}

int main()
{
    auto po1 = ParseLatexToF90("a_0x + \\frac{a_1a_2\\texttt{gauss_x}(-x^2)}{a_3} + 2^\\frac{b_0 - \\log(b_1x)}{2\\pi\\sqrt{c_0x}+c_1}");
    auto po2 = ParseLatexToF90("\\frac{1}{a_0\\sqrt{2\\pi}}\\exp(-\\frac{1}{2}(\\frac{x-x_0}{s})^2)");
    auto po3 = ParseLatexToF90("2.34^x_dx_1");
    auto po4 = ParseLatexToF90("2\\texttt{test_func}(2.71, 1.61\\frac{\\sqrt{3.14\\texttt{test_func}(x - x_0, y))}}{\\exp(x+y)}");

    CheckParseValidity(&po4, "/home/prime/.nested_fit/func_names.dat");
    
    if(CheckError(&po4))
    {
        char buffer[128];
        GetErrorMsg(&po4, buffer);
        std::cout << buffer << std::endl;
    }
    
    FreeParseOutput(&po1);
    FreeParseOutput(&po2);
    FreeParseOutput(&po3);
    FreeParseOutput(&po4);

    return 0;
}
