#include <stdio.h>
#include <stdlib.h>
#include <argp.h>

#include <iostream>
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
static char args_doc[] = "LABEL_FILE TEMPLATE_FILE CORPUS_FILE DATA_FILE"; /* A description of the arguments we accept. */

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
    char *label_file, *template_file, *corpus_file, *data_file, *netloc_file, *output_file;
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
            if (state->arg_num == 3) arguments->corpus_file = arg;
            if (state->arg_num == 4) arguments->data_file = arg;
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

int main(int argc, char *argv[])
{
    char *label_file, *template_file, *corpus_file, *data_file, *netloc_file, *output_file;
    bool verbose, silent, debug, profile;
    
    struct arguments arguments;
    arguments.label_file = NULL;
    arguments.template_file = NULL;
    arguments.corpus_file = NULL;
    arguments.data_file = NULL;
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

    return 0;
}

void load_label_file(const char *label_file, map<string, string> &sub_parent_map, map<string, int> &tag_label_map, map<int, string> &label_tag_map)
{
    Json::Value label_dict;
    ifstream input(label_file);
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
