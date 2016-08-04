#include <stdio.h>
#include <stdlib.h>
#include <argp.h>
#include <regex.h>

#include <iostream>
#include <stdexcept>
#include <fstream>
#include <string>
#include <vector>
#include <map>

#include <json/json.h>
#include <json/json-forwards.h>

using namespace std;

const char *argp_program_version = "feature 0.1";
const char *argp_program_bug_address = "<SnakeHunt2012@gmail.com>";

static char prog_doc[] = "Compile feature matrix from corpus."; /* Program documentation. */
static char args_doc[] = "LABEL_FILE TEMPLATE_FILE TRAIN_FILE VALIDATE_FILE MODEL_FILE"; /* A description of the arguments we accept. */

/* Keys for options without short-options. */
#define OPT_DEBUG       1
#define OPT_PROFILE     2
#define OPT_NETLOC_FILE 3

/* The options we understand. */
static struct argp_option options[] = {
    {"verbose",     'v',             0,             0, "produce verbose output"},
    {"quite",       'q',             0,             0, "don't produce any output"},
    {"silent",      's',             0,             OPTION_ALIAS},

    {0,0,0,0, "The following options are about algorithm:" },
    
    {"netloc-file", OPT_NETLOC_FILE, "NETLOC_FILE", 0, "netloc dict file in json format"},

    {0,0,0,0, "The following options are about output format:" },
    
    {"output",      'o',             "FILE",        0, "output information to FILE instead of standard output"},
    {"debug",       OPT_DEBUG,       0,             0, "output debug information"},
    {"profile",     OPT_PROFILE,     0,             0, "output profile information"},
    
    { 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments
{
    char *label_file, *template_file, *train_file, *validate_file, *model_file, *netloc_file, *output_file;
    bool verbose, silent, debug, profile;
};

/* Parse a single option. */
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = (struct arguments *) state->input;
    switch (key)
        {
        case 'v':
            arguments->verbose = true;
            break;
        case 'q': case 's':
            arguments->silent = true;
            break;
        case 'o':
            arguments->output_file = arg;
            break;
        case OPT_NETLOC_FILE:
            arguments->netloc_file = arg;
            break;
        case OPT_DEBUG:
            arguments->debug = true;
            break;
        case OPT_PROFILE:
            arguments->profile = true;
            break;
            
        case ARGP_KEY_ARG:
            if (state->arg_num == 0) arguments->label_file = arg;
            if (state->arg_num == 1) arguments->template_file = arg;
            if (state->arg_num == 2) arguments->train_file = arg;
            if (state->arg_num == 3) arguments->validate_file = arg;
            if (state->arg_num == 4) arguments->model_file = arg;
            if (state->arg_num >= 5) argp_usage(state);
            break;
            
        case ARGP_KEY_END:
            if (state->arg_num < 2) argp_usage(state);
            break;
            
        case ARGP_KEY_NO_ARGS:
            argp_usage(state);
            break;
            
        default:
            return ARGP_ERR_UNKNOWN;
        }
    return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, prog_doc };

void load_label_file(const char *, map<string, string> &, map<string, int> &, map<int, string> &);
void load_template_file(const char *, map<string, double> &, map<string, int> &, map<int, string> &);
void load_netloc_file(const char *, map<string, int> &, map<int, string> &);
void load_data_file(const char *);
regex_t compile_regex(const char *);
string regex_search(const regex_t *, int, const string &);
string regex_replace(const regex_t *, const string &, const string &);

int main(int argc, char *argv[])
{
    char *label_file, *template_file, *train_file, *validate_file, *data_file, *netloc_file, *output_file;
    bool verbose, silent, debug, profile;
    
    struct arguments arguments;
    arguments.label_file = NULL;
    arguments.template_file = NULL;
    arguments.train_file = NULL;
    arguments.validate_file = NULL;
    arguments.model_file = NULL;
    arguments.netloc_file = NULL;
    arguments.output_file = NULL;
    arguments.verbose = true;
    arguments.silent = false;
    arguments.debug = false;
    arguments.profile = false;
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    map<string, string> sub_parent_map;
    map<string, int> tag_label_map;
    map<int, string> label_tag_map;
    
    load_label_file(arguments.label_file, sub_parent_map, tag_label_map, label_tag_map);

    map<string, double> word_idf_map;
    map<string, int> word_index_map; 
    map<int, string> index_word_map;

    load_template_file(arguments.template_file, word_idf_map, word_index_map, index_word_map);

    map<string, int> netloc_index_map;
    map<int, string> index_netloc_map;

    load_netloc_file(arguments.netloc_file, netloc_index_map, index_netloc_map);

    load_data_file(arguments.train_file);
    //load_data_file(arguments.validate_file);

    return 0;
}

void load_label_file(const char *label_file, map<string, string> &sub_parent_map, map<string, int> &tag_label_map, map<int, string> &label_tag_map)
{
    Json::Value label_dict;
    ifstream input(label_file);
    if (!input)
        throw runtime_error("error: unable to open input file: " + string(label_file));
    input >> label_dict;

    const Json::Value sub_parent_dict = label_dict["parent_map"];
    const Json::Value tag_label_dict = label_dict["tag_label_dict"];
    const Json::Value label_tag_dict = label_dict["label_tag_dict"];

    Json::Value::Members key_vec;
    
    key_vec = sub_parent_dict.getMemberNames();
    for (Json::Value::Members::const_iterator iter = key_vec.begin(); iter != key_vec.end(); ++iter)
        sub_parent_map[*iter] = sub_parent_dict[*iter].asString();
    
    key_vec = tag_label_dict.getMemberNames();
    for (Json::Value::Members::const_iterator iter = key_vec.begin(); iter != key_vec.end(); ++iter)
        tag_label_map[*iter] = tag_label_dict[*iter].asInt();
    
    key_vec = label_tag_dict.getMemberNames();
    for (Json::Value::Members::const_iterator iter = key_vec.begin(); iter != key_vec.end(); ++iter)
        label_tag_map[atoi(((string) *iter).c_str())] = label_tag_dict[*iter].asString();
}

void load_template_file(const char *template_file, map<string, double> &word_idf_map, map<string, int> &word_index_map, map<int, string> &index_word_map)
{
    Json::Value template_dict;
    ifstream input(template_file);
    if (!input)
        throw runtime_error("error: unable to open input file: " + string(template_file));
    input >> template_dict;

    const Json::Value word_idf_dict = template_dict["word_idf_dict"];
    const Json::Value word_index_dict = template_dict["word_index_dict"];
    const Json::Value index_word_dict = template_dict["index_word_dict"];

    vector<string> key_vec;

    key_vec = word_idf_dict.getMemberNames();
    for (vector<string>::const_iterator iter = key_vec.begin(); iter != key_vec.end(); ++iter)
        word_idf_map[*iter] = atof(word_idf_dict[*iter].asString().c_str());

    key_vec = word_index_dict.getMemberNames();
    for (vector<string>::const_iterator iter = key_vec.begin(); iter != key_vec.end(); ++iter)
        word_index_map[*iter] = word_index_dict[*iter].asInt();

    key_vec = index_word_dict.getMemberNames();
    for (vector<string>::const_iterator iter = key_vec.begin(); iter != key_vec.end(); ++iter)
        index_word_map[atoi(((string) (*iter)).c_str())] = index_word_dict[*iter].asString();
}

void load_netloc_file(const char *netloc_file, map<string, int> &netloc_index_map, map<int, string> &index_netloc_map)
{
    Json::Value netloc_dict;
    ifstream input(netloc_file);
    if (!input)
        throw runtime_error("error: unable to open input file: " + string(netloc_file));
    input >> netloc_dict;

    const Json::Value netloc_index_dict = netloc_dict["netloc_index_dict"];
    const Json::Value index_netloc_dict = netloc_dict["index_netloc_dict"];

    vector<string> key_vec;

    key_vec = netloc_index_dict.getMemberNames();
    for (vector<string>::const_iterator iter = key_vec.begin(); iter != key_vec.end(); ++iter)
        netloc_index_map[*iter] = netloc_index_dict[*iter].asInt();
    
    key_vec = index_netloc_dict.getMemberNames();
    for (vector<string>::const_iterator iter = key_vec.begin(); iter != key_vec.end(); ++iter)
        index_netloc_map[atoi(((string) (*iter)).c_str())] = index_netloc_dict[*iter].asString();
}

void load_data_file(const char *data_file)
{
    ifstream input(data_file);
    if (!input)
        throw runtime_error("error: unable to open input file: " + string(data_file));

    regex_t tag_regex = compile_regex("(<LBL>)(.*)(</LBL>)");
    regex_t title_regex = compile_regex("(<TITLE>)(.*)(</TITLE>)");
    regex_t url_regex = compile_regex("(<URL>)(.*)(</URL>)");
    regex_t content_regex = compile_regex("(<CONTENT>)(.*)(</CONTENT>)");
    regex_t image_regex = compile_regex("\\[img\\][^\\[]*\\[/img\\]");
    regex_t br_regex = compile_regex("\\[br\\]");
    
    string line;
    while (getline(input, line)) {
        string tag = regex_search(&tag_regex, 2, line);
        string title = regex_search(&title_regex, 2, line);
        string url = regex_search(&url_regex, 2, line);
        string content = regex_search(&content_regex, 2, line);
        content = regex_replace(&image_regex, "", content);
        content = regex_replace(&br_regex, " ", content);
    }
}

regex_t compile_regex(const char *pattern)
{
    regex_t regex;
    int error_code = regcomp(&regex, pattern, REG_EXTENDED);
    if (error_code != 0) {
        size_t length = regerror(error_code, &regex, NULL, 0);
        char *buffer = (char *) malloc(sizeof(char) * length);
        (void) regerror(error_code, &regex, buffer, length);
        string error_message = string(buffer);
        free(buffer);
        throw runtime_error(string("error: unable to compile regex '") + pattern + "', message: " + error_message);
    }
    return regex;
}

string regex_search(const regex_t *regex, int field, const string &line)
{
    regmatch_t match_res[field + 1];
    int error_code = regexec(regex, line.c_str(), field + 1, match_res, 0);
    if (error_code != 0) {
        size_t length = regerror(error_code, regex, NULL, 0);
        char *buffer = (char *) malloc(sizeof(char) * length);
        (void) regerror(error_code, regex, buffer, length);
        string error_message = string(buffer);
        free(buffer);
        throw runtime_error(string("error: unable to execute regex, message: ") + error_message);
    }
    return string(line, match_res[field].rm_so, match_res[field].rm_eo - match_res[field].rm_so);
}

string regex_replace(const regex_t *regex, const string &sub_string, const string &ori_string)
{
    regmatch_t match_res;
    int error_code;
    string res_string = ori_string;
    while ((error_code = regexec(regex, res_string.c_str(), 1, &match_res, 0)) != REG_NOMATCH) {
        if (error_code != 0) {
            size_t length = regerror(error_code, regex, NULL, 0);
            char *buffer = (char *) malloc(sizeof(char) * length);
            (void) regerror(error_code, regex, buffer, length);
            string error_message = string(buffer);
            free(buffer);
            throw runtime_error(string("error: unable to execute regex, message: ") + error_message);
        }
        cout << string(res_string, match_res.rm_so, match_res.rm_eo - match_res.rm_so) << endl;
        res_string = string(res_string, 0, match_res.rm_so) + sub_string + string(res_string, match_res.rm_eo, res_string.size() - match_res.rm_eo);
    }
    return res_string;
}
