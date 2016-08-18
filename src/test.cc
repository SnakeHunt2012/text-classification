#include "classifier.h"

int main()
{
    char* classify2_label_file = "./final-label-dict.json";
    char* classify2_template_file = "./template-final-5000.json";
    char* classify2_netloc_file = "./netloc-dict.json";
    char* classify2_model_file = "./model";

    Classifier* classify2 = new Classifier(classify2_label_file,
                                           classify2_template_file,
                                           classify2_netloc_file,
                                           classify2_model_file);


    return 0;
}
