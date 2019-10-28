 #include "configure.h"
 string get_config(string key) {
    ifstream ifs;
    ifs.open(CONFIG_FILE);
    if(ifs.fail()) {
        cerr << "Error: cannot open config file '" << CONFIG_FILE << "'" << endl;
        ifs.close();
        exit(-1);
    }
    string entry, value;
    while(ifs >> entry >> value) {
        if(entry == key) {
            ifs.close();
            return value;
        }
    }
    ifs.close();
    cerr << "Config error: cannot find " << key << endl;
    exit(-1);
}