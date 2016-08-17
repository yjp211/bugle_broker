#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <json-c/json.h>
#include <json-c/json_object.h>

#include "config_opt.h"

//配置文件最大的长度
#define BUF_SIZE  10240 


int parse_json(struct json_object *config_json, struct config_t *config)
{
    char *key = NULL;
    struct json_object_iter iter;
    struct json_object *broker_json = NULL;
    struct json_object *scache_json = NULL;

    json_object_object_get_ex(config_json, "Broker", &broker_json);
    if(NULL == broker_json){
        fprintf(stderr, "parse config failed, %m\n");
        return -1;
    }
    
    json_object_object_foreachC(broker_json, iter){
        key = iter.key;
        if(strcmp(key, "WithWebSockets") == 0){
            config->with_websockets = json_object_get_int(iter.val);
        
        }else if(strcmp(key, "RpcPort") == 0){
            config->rpc_port= json_object_get_int(iter.val);
        }else if(strcmp(key, "BrokerPort") == 0){
            config->broker_port = json_object_get_int(iter.val);
        }else if(strcmp(key, "WebSocketPort") == 0){
            config->webscoket_port = json_object_get_int(iter.val);
        
        }else if(strcmp(key, "ListenCount") == 0){
            config->listen_count = json_object_get_int(iter.val);
        
        }else if(strcmp(key, "RpcThreadCount") == 0){
            config->rpc_thread_count = json_object_get_int(iter.val);
        }else if(strcmp(key, "BrokerThreadCount") == 0){
            config->broker_thread_count = json_object_get_int(iter.val);
        
        }else if(strcmp(key, "LogPath") == 0){
            config->log_path = strdup(json_object_get_string(iter.val));
        }else if(strcmp(key, "LogMaxSize") == 0){
            config->log_max_size = json_object_get_int(iter.val);
        }else if(strcmp(key, "DebugLevel") == 0){
            config->debug_level = json_object_get_int(iter.val);
        
        
        }else if(strcmp(key, "ClientInitCount") == 0){
            config->client_init_count = json_object_get_int(iter.val);
        }else if(strcmp(key, "ClientInrcStep") == 0){
            config->client_inrc_step = json_object_get_int(iter.val);
        
        }else if(strcmp(key, "TopicInitCount") == 0){
            config->topic_init_count = json_object_get_int(iter.val);
        }else if(strcmp(key, "TopicInrcStep") == 0){
            config->topic_inrc_step = json_object_get_int(iter.val);
        
        }else if(strcmp(key, "SubInitCount") == 0){
            config->sub_init_count = json_object_get_int(iter.val);
        }else if(strcmp(key, "SubInrcStep") == 0){
            config->sub_inrc_step = json_object_get_int(iter.val);
        
        }else if(strcmp(key, "CheckReferInterval") == 0){
            config->check_refer_interval = json_object_get_int(iter.val);
        }else if(strcmp(key, "CheckReferCount") == 0){
            config->check_refer_count = json_object_get_int(iter.val);
        }else if(strcmp(key, "CheckReferTimeoutAction") == 0){
            config->check_refer_timeout_action = json_object_get_int(iter.val);
        }
    }

    return 0;
}

int parse_config(char *config_path, struct config_t *config)
{
    int rc = 0;
    int fd = 0;
    int read_len = 0;
    char buf[BUF_SIZE];
    
    struct json_object *config_json = NULL;

    if((fd = open(config_path, O_RDONLY)) <= 0){
        fprintf(stderr, "Open config file<%s> failed, %m\n", config_path);
        return -1;
    }

    bzero(buf, BUF_SIZE);
    read_len = read(fd, buf, BUF_SIZE);
    if(read_len == BUF_SIZE){
        fprintf(stderr, "Config file is too big\n");
        return -1;
    }else if(read_len == 0){
        fprintf(stderr, "Config file is empty\n");
        return -1;
    
    }else if(read_len < 0){
        fprintf(stderr, "Config file read error, %m\n");
        return -1;
    }

    config_json = json_tokener_parse(buf);
    if(NULL == config_json) {
        fprintf(stderr, "Config file can't format as json, %m\n");
        return -1;
    }

    rc = parse_json(config_json, config);
    json_object_put(config_json);

    return rc;
}
