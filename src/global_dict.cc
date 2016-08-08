#include <iostream>
#include <fstream>
#include <stdexcept>

#include <stdlib.h>

#include "json/json.h"
#include "json/json-forwards.h"

#include "global_dict.h"

using namespace std;

GlobalDict::GlobalDict(const char *label_file, const char *template_file, const char *netloc_file)
{
    load_label_file(label_file);
    load_template_file(template_file);
    load_netloc_file(netloc_file);
}

void GlobalDict::load_label_file(const char *label_file)
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

void GlobalDict::load_template_file(const char *template_file)
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

void GlobalDict::load_netloc_file(const char *netloc_file)
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

