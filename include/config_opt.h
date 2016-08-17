#ifndef _CONFIG_OPT_H_
#define _CONFIG_OPT_H_

struct config_t {
    int with_websockets;
    int rpc_port;
    int broker_port;
    int webscoket_port;
    
    int listen_count;
    
    int rpc_thread_count;
    int broker_thread_count;


    char *log_path;
    int log_max_size;
    int debug_level;

    
    int client_init_count;
    int client_inrc_step;

    int topic_init_count;
    int topic_inrc_step;
    
    int sub_init_count;
    int sub_inrc_step;

    //free connect handle should check whether it's have refer 
    int check_refer_interval;
    int check_refer_count;
    //0:  means ignore timeout, free connect go on
    //1:  means give up free, this is save, but will case mem leak
    int check_refer_timeout_action;

};

int parse_config(char *config_path, struct config_t *config);

#endif
