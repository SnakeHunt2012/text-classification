#ifndef CLASSIFIER_H_
#define CLASSIFIER_H_

#include <string>
#include <vector>
#include <map>

class Classifier {
public:
    
    Classifier(const char *label_file, const char *template_file, const char *netloc_file, const char *model_file);
    ~Classifier();

    void classify(const std::string &url, const std::map<std::string, int> &title, const std::map<std::string, int> &content, std::string &tag, float *proba);
    
private:

    void *global_dict;
    void *model;
};

#endif  // CLASSIFIER_H_
