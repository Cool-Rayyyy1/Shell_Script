/**
 * shell
 * CS 241 - Fall 2020
 */
#include "format.h"
#include "shell.h"
#include "vector.h"
#include "unistd.h"
#include "stdio.h"
#include "string.h"
#include "sstring.h"
#include "ctype.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
int cont_distribution(char* buff);
int stop_handle(char* buff);
int kill_distribution(char* buff);
void history_distribution();
void handle_history(char* buff);
char **strsplit(char* buff, char delim);
int extend_execution(char* buff, char* redic_file, int signal);
int buildin_execution(char* buff);
void and_distribution(char* buff);
void or_distribution(char* buff);
void separator_distribution(char* buff);
int cd_distribution(char* buff);
void jinghao_distribution(char* buff);
void prefix_distribution(char* buff);
int operator_execution(char*buff);
vector* load_f(char* path);
void load_in_his(char* path);
void free_process(pid_t pid);
int redirection_execution(char* buff);



typedef struct process {
    char *command;
    pid_t pid;
} process;

process_info* ps_distribution(process* curr_process);
process* process_vec_create(char* buff, pid_t curr_pid);
static vector* history_vector;
static vector* process_vec;

//helper function here 

void free_process(pid_t pid) {
    for (size_t i = 0; i < vector_size(process_vec); i++) {
            process* shut_down = (process*) vector_get(process_vec, i);
            if (shut_down->pid == pid) {
                //printf("has been deleted");
                free(shut_down->command);
                free(shut_down);
                shut_down = NULL;
                vector_erase(process_vec, i);
                break;
                
            }
    }
}
process_info* ps_distribution(process* curr_process) {
    
    char* buff = NULL;
    size_t capacity = 0;
    int judge = 0;
    process_info* curr_info = calloc(1, sizeof(process_info));
    char target_path[50];
    //真尼玛犯了个sb错误
    snprintf(target_path, 50, "/proc/%d/status", curr_process->pid);
    FILE *temp = fopen(target_path, "r");
    if (!temp) {
        //printf("111");
        print_script_file_error();
        //printf("111");
        return NULL;
    }
    // 初始化所有的值
    curr_info->pid = curr_process->pid;
    curr_info->command = curr_process->command;
    curr_info->time_str = NULL;
    curr_info->start_str = NULL;
    //printf("111");
    //-----------------------------------------------------
    while((judge = getline(&buff, &capacity, temp)) != -1) {
        if (buff[strlen(buff) - 1] == '\n') {
            buff[strlen(buff) - 1] = 0;
        }
        char** split_str = strsplit(buff, ':');
        //char* rotator = split_str[1];
        if (!strcmp(split_str[0], "State")) {
            
            char* temp_state = strdup(split_str[1]);
            if (isspace(*temp_state) != 0) {
                temp_state = temp_state + 1;
            }
            curr_info->state = *temp_state;
            /**char* temp = split_str[1];
            while(*temp) {
                printf("%c", *temp);
                temp++;
            }
            **/
            //printf("state is %c\n", *split_str[1]);
            //curr_info->state = *split_str;
        } else if (!strcmp(split_str[0], "Threads")) {
            char* temp_threads = strdup(split_str[1]);
            if (isspace(*temp_threads) != 0) {
                temp_threads = temp_threads + 1;
            }
            curr_info->nthreads = atol(temp_threads);
        } else if (!strcmp(split_str[0], "VmSize")) {
            char* temp_vmsize = strdup(split_str[1]);
            if (isspace(*temp_vmsize) != 0) {
                temp_vmsize = temp_vmsize + 1;
            }
            curr_info->vsize = atol(temp_vmsize);
        }
    }
    fclose(temp);
    //printf("222");
    //----------------------------------------------------
    snprintf(target_path, 40, "/proc/%d/stat", curr_process->pid);
    FILE* temp2 = fopen(target_path, "r");
    if (!temp2) {
        print_script_file_error();
        return NULL;
    }
    // reinialize
    unsigned long utime = 0;
    unsigned long stime = 0;
    unsigned long start_t = 0;
    unsigned long btime = 0;
    capacity = 0;
    int count = 1;
    
    while ((judge = getline(&buff, &capacity, temp2)) != -1) {
        char* str_copy = strdup(buff);
        char* for_use = strtok(str_copy, " ");
        while (for_use != NULL) {
            if (count == 14) {
                utime = atol(for_use);
            } else if (count == 15) {
                stime = atol(for_use);
            } else if (count == 22) {
                start_t = atol(for_use);
            }
            for_use = strtok (NULL, " ");
            count++;
        }
    }
    fclose(temp2);
    //----------------------------------------------------------------
    //printf("333");
    capacity = 0;
    FILE *temp3 = fopen("/proc/stat","r");
    if (!temp3) {
        print_script_file_error();
        return NULL;
    }
    
    while ((judge = getline(&buff, &capacity, temp3)) != -1) {
        char** temp10 = strsplit(buff, ' ');
        if(!strcmp(temp10[0], "btime")) {
            char* compare = strdup(temp10[1]);
            if (isspace(*compare)) {
                compare = compare + 1;
            }
            btime = atol(compare);
        } 
    }
    fclose(temp3);
    free(buff);
    buff = NULL;


    //处理time——str
    char* temp5 = malloc(50);
    unsigned long final_runtime = (utime + stime) / sysconf(_SC_CLK_TCK);
    unsigned long param1 = (final_runtime / 60);
    unsigned long param2 = (final_runtime % 60);
    execution_time_to_string(temp5, 50, param1, param2);
    curr_info->time_str = temp5;


    //处理start_str
    char* temp6 = malloc(20);
    time_t total_= start_t/sysconf(_SC_CLK_TCK) + btime;
    struct tm *s_time = localtime(&total_);
    time_struct_to_string(temp6, 20, s_time);
    curr_info->start_str = temp6;
    
    return curr_info;

}


int cont_distribution(char* buff) {
    //handle history here
    handle_history(buff);
    char** split_buff = strsplit(buff, ' ');
    if (split_buff[1] == NULL) {
        print_invalid_command(split_buff[0]);
        return 0;
    }
    pid_t curr_pid = atoi(split_buff[1]);
    for (size_t i = 0; i < vector_size(process_vec); i++) {
        process* curr_pro = (process*)vector_get(process_vec, i);
        if (curr_pro->pid == curr_pid) {
            kill(curr_pro->pid, SIGCONT);
            print_continued_process(curr_pid, curr_pro->command);
            return 1;
        }
    }
    // havent find
    print_no_process_found(curr_pid);
    return 0;
}



int kill_distribution(char* buff) {
    handle_history(buff);
    char** split_buff = strsplit(buff, ' ');
    if (split_buff[1] == NULL) {
        print_invalid_command(split_buff[0]);
        return 0;
    }
    pid_t curr_pid = atoi(split_buff[1]);
    for (size_t i = 0; i < vector_size(process_vec); i++) {
        process* curr_pro = (process*)vector_get(process_vec, i);
        if (curr_pro->pid == curr_pid) {
            kill(curr_pro->pid,SIGKILL);
            free_process(curr_pid);
            print_killed_process(curr_pid, buff);
            return 1;
        }
    }
    // havent find
    print_no_process_found(curr_pid);
    return 0;
}



process* process_vec_create(char* buff, pid_t curr_pid) {
    process* curr_process = malloc(1*sizeof(process));
    curr_process->command = buff;
    curr_process->pid = curr_pid;
    return curr_process;
}

vector* load_f(char* path) {
    FILE* temp = NULL;
    temp = fopen(path, "r");
    if (temp == NULL) {
        print_script_file_error();
    }
    vector* file_vec = string_vector_create();
    char* buff = NULL;
    size_t capacity = 0;
    ssize_t result = 0;
    while((result = getline(&buff, &capacity, temp)) != -1) {
        if (buff[strlen(buff) - 1] == '\n'){
            buff[strlen(buff) - 1] = '\0';
        }
        vector_push_back(file_vec, buff);
    }
    fclose(temp);
    free(buff);
    return file_vec;
}


void load_in_his(char* path){
    FILE* temp = NULL;
    temp = fopen(path, "r");
    if (temp == NULL) {
        print_history_file_error();
        return;
    }
    char* buff = NULL;
    size_t capacity = 0;
    while(getline(&buff, &capacity, temp) != -1) {
        if (buff[strlen(buff) - 1] == '\n'){
            buff[strlen(buff) - 1] = '\0';
        }   
        vector_push_back(history_vector, buff);
    }
    free(buff);
    fclose(temp);
    return;
}


void history_distribution() {
    for (size_t i = 0; i < vector_size(history_vector); i++) {
        print_history_line(i, vector_get(history_vector,i));
    }
}



void handle_history(char* buff) {
    vector_push_back(history_vector, buff);
}


char **strsplit(char* buff, char delim) {
  sstring* sstring = cstr_to_sstring(buff); 
  vector* args = sstring_split(sstring, delim); 
  sstring = NULL;
  char** arr = malloc(sizeof(char*)*(vector_size(args)+1));
  for (size_t i = 0; i < vector_size(args); i++) {
    arr[i] = strdup(vector_get(args, i));
  }
  arr[vector_size(args)] = NULL;
  return arr;
}




int extend_execution(char* buff, char* redic_file, int signal) {
    //part1的扩展命令
    char* buff_temp = strdup(buff);
    int background_judge = 0;
    //part2 background_process
    if (buff[strlen(buff) - 1] == '&') {
        background_judge = 1;
        
    }
    char** new_buff = strsplit(buff_temp, ' ');
    pid_t pid = fork();
    if (background_judge) {
        //part2 background_execution
        background_judge = 0;
        if (pid == -1) {
            //fork failed
            print_fork_failed();
            return -1;
        } else if (pid > 0) {
            //parents process
            //printf("pid is %d\n", pid);
            process* curr_process = process_vec_create(buff_temp, pid);
            //printf("111current pid is %d\n", curr_process->pid);
            vector_push_back(process_vec, curr_process);
            //for (unsigned i = 0; i < vector_size(process_vec); i++) {
                //process* temp = (process*) vector_get(process_vec, i);
                //printf("2222current pid is %d\n", temp->pid); 
            //}
            if (setpgid(pid, pid) == -1) { // failed
                print_setpgid_failed();
                exit(1);
            }
            return 1;
        } else {
            //child process
            //pid_t current_pid = getpid();
            //if (setpgid(current_pid, getpid()) == -1) {
                //print_setpgid_failed();
            //}
            sstring* sstr = cstr_to_sstring(buff);
            vector* curr_command = sstring_split(sstr, ' ');
            if (signal == 3) {
                char* file_path = get_full_path(redic_file);
                FILE* f_file = freopen(file_path, "r", stdin);
                if (!f_file) {
                    print_redirection_file_error();
                    exit(1);
                }
                free(file_path);
            }
            char* arg[vector_size(curr_command)];
            arg[vector_size(curr_command) - 1] = NULL;
            for (unsigned i = 0; i < vector_size(curr_command) - 1; i++) {
                arg[i] = (char*) vector_get(curr_command, i);
            }
            print_command_executed(getpid());
            //printf("first is : %s\n", new_buff[0]);
            //printf("second is: %s\n", new_buff[1]);
            if (signal == 1) {
                char* file_path = get_full_path(redic_file);
                int f_temp = open(file_path, O_WRONLY | O_TRUNC | O_CREAT, 0666);
                free(file_path);
                if (f_temp == -1) {
                    print_redirection_file_error();
                    exit(1);
                }
                dup2(f_temp, 1);
            } else if (signal == 2) {
                char* file_path = get_full_path(redic_file);
                FILE* f_file = freopen(file_path, "a", stdout);
                if (!f_file) {
                    print_redirection_file_error();
                    exit(1);
                }
                free(file_path);
            }
            execvp(arg[0], arg);
            print_exec_failed(buff);
            for (size_t i = 0; i < vector_size(process_vec); i++) {
                process* shut_down = (process*) vector_get(process_vec, i);
                if (shut_down->pid == pid) {
                    //printf("has been deleted");
                    free(shut_down->command);
                    free(shut_down);
                    shut_down = NULL;
                    vector_erase(process_vec, i);
                    break;
                }
            }
            return -1;
        } 

    } else {
        if (pid == -1) {
        //fork failed
            print_fork_failed();
            return -1;
        } else if (pid == 0) {
            //child process
            if (signal == 3) {
                char* file_path = get_full_path(redic_file);
                FILE* f_file = freopen(file_path, "r", stdin);
                if (!f_file) {
                    print_redirection_file_error();
                    exit(1);
                }
                free(file_path);
            }
            pid_t current_pid = getpid();
            if (setpgid(current_pid, getpid()) == -1) {
                print_setpgid_failed();
            }
            print_command_executed(getpid());
            if (signal == 1) {
                char* file_path = get_full_path(redic_file);
                int f_temp = open(file_path, O_WRONLY | O_TRUNC | O_CREAT, 0666);
                free(file_path);
                if (f_temp == -1) {
                    print_redirection_file_error();
                    exit(1);
                }
                dup2(f_temp, 1);
            } else if (signal == 2) {
                char* file_path = get_full_path(redic_file);
                FILE* f_file = freopen(file_path, "a", stdout);
                if (!f_file) {
                    print_redirection_file_error();
                    exit(1);
                }
                free(file_path);
            }
            execvp(new_buff[0], &new_buff[0]);
            print_exec_failed(buff);
            free_process(getpid());
            return -1;
        } else {
            //parent process
            //handle_history(buff);
            //pid_t current_pid = getpid();
            process* curr_process = process_vec_create(buff_temp, pid);
            vector_push_back(process_vec, curr_process);
            if (setpgid(pid, getpid()) == -1) {
                print_setpgid_failed();
                exit(1);
            }
            int status;
           
            pid_t judge = waitpid(pid, &status, 0);
            //printf("status is %d\n", status);
            //printf("false is %d/n", judge);
            /**
            if (judge == -1) {
                print_wait_failed();
                return -1;
            } else if (WIFSIGNALED(status)) {
                // 子进程被命令停止
                return -1;
            } else if (WIFEXITED(status)) {
                return 1;
            }
            **/
            if (judge != -1) {
            //子进程调用成功
                if (WIFEXITED(status) == 0 || WEXITSTATUS(status) != 0) {
                    return -1;
                }
            } else {
                //子进程调用失败
                print_wait_failed();
                return -1;
            }
        }
        return 1;
    }
    
}


int stop_handle(char* buff) {
    handle_history(buff);
    char** split_buff = strsplit(buff, ' ');
    if (split_buff[1] == NULL) {
        print_invalid_command(split_buff[0]);
        return 0;
    }
    pid_t curr_pid = atoi(split_buff[1]);
    for (size_t i = 0; i < vector_size(process_vec); i++) {
        process* curr_pro = (process*)vector_get(process_vec, i);
        if (curr_pro->pid == curr_pid) {
            kill(curr_pro->pid, SIGSTOP);
            print_stopped_process(curr_pid, curr_pro->command);
            return 1;
        }
    }
    // havent find
    print_no_process_found(curr_pid);
    return 0;
}

int buildin_execution(char* buff) {
    //可以直接查空格分开后的第一位
    //包含所有的情况
    //printf("666");
    char** new_buff = strsplit(buff, ' ');
    //printf("555");
    if (strcmp(new_buff[0], "cd") == 0) {
        //printf("777\n");
        int temp = -100;
        //printf("888\n");
        temp = cd_distribution(buff);
        handle_history(buff);
        return temp;
    } else if (strcmp(buff, "!history\0") == 0) {
        history_distribution();
        return 1;
    } else if (*buff == '#') {
        jinghao_distribution(buff);
        //handle_history(buff);
        return 1;
    } else if (*buff == '!') {
        prefix_distribution(buff);
        //stupid bug
        //handle_history(buff);
        return 1;
    //week2 build in command
    } else if (strcmp(new_buff[0], "stop") == 0) {
        int temp1 = -100;
        temp1 = stop_handle(buff);
        return temp1;
    } else if (strcmp(new_buff[0], "cont") == 0) {
        return cont_distribution(buff);
        
    } else if (strcmp(new_buff[0], "kill") == 0) {
        return kill_distribution(buff);
    } else if (strcmp(buff, "ps") == 0) {
        if (strlen(buff) > 2) {
            print_invalid_command(buff);
            return 0;
        }
        for (int i = vector_size(process_vec) - 1; i >= 0; i--) {
            process* curr = (process*) vector_get(process_vec, i);
            if (getpgid(curr->pid) != getpid()) {
                int s;
                int wait = waitpid(curr->pid, &s, WNOHANG);
                if (wait == 0) {
                    continue;
                } else if (wait == -1) {
                    print_wait_failed();
                    exit(1);
                } else { 
                    free_process(curr->pid);
                }
            } 
        }
        print_process_info_header();
        for (size_t i = 0; i < vector_size(process_vec); i++) {
            print_process_info(ps_distribution((process*) vector_get(process_vec, i)));

        }    
        handle_history(buff);
        return 1;
    }
    //build in execetion 没有找到
    //需要执行拓展命令
    int temp = 0;
    temp =  extend_execution(buff, NULL, 4);
    if (temp == 1) {
        handle_history(buff);
        //handle history here;
    }
    return temp;
}  



void and_distribution(char* buff){
    sstring* my_sstr = cstr_to_sstring(buff);
    vector* answer = string_vector_create();
    if (sstring_substitute(my_sstr, 0, " && ", "*") == 0){
        answer = sstring_split(my_sstr, '*');
    }
    int temp = -100;
    temp = buildin_execution(vector_get(answer, 0));
    if (temp != 1){
        //printf("111\n");
        return;
    } 
    //printf("222\n");
    buildin_execution(vector_get(answer, 1));
}



void or_distribution(char* buff){
    

    sstring* my_sstr = cstr_to_sstring(buff);
    vector* answer = string_vector_create();
    if (sstring_substitute(my_sstr, 0, " || ", "*") == 0){
        answer = sstring_split(my_sstr, '*');
    }
    int temp = -100;
    temp = buildin_execution(vector_get(answer, 0));
    if (temp == 1){
        return;
    }
    buildin_execution(vector_get(answer, 1));
    
}


void separator_distribution(char* buff){
    
    sstring* my_sstr = cstr_to_sstring(buff);
    vector* answer = string_vector_create();
    if (sstring_substitute(my_sstr, 0, "; ", "*") == 0){
        answer = sstring_split(my_sstr, '*');
    }
    buildin_execution(vector_get(answer, 0));
    buildin_execution(vector_get(answer, 1));
}



int cd_distribution(char* buff) {
    char** new_buff = strsplit(buff, ' ');
    //new_buff 应该是 cd  xxx 
    // A missing path should be treated as a nonexistent directory.
    char** rotator = new_buff;
    int count = 0;
    while(*rotator) {
        count++;
        rotator++;
    }
    //printf("length is %d\n", count);
    //char* judge = new_buff[1];
    if (count == 1) {
        //printf("544\n");
        print_no_directory(buff);
        return -1;
    }
    char* judge = new_buff[1];
    if (chdir(judge) != 0) {
        print_no_directory(judge);
        return -1;
    }
    return 1;
}


void jinghao_distribution(char* buff) {
    //检查#后面不是空格
    int length = strlen(buff);
    if (length == 1) {
        print_invalid_command(buff);
        return;
    }
    //检查#后面都是数字
    char* rotator = buff + 1;
    bool temp = false;
    while(*rotator) {
        if (isdigit(*rotator)) {
            temp = false;
            rotator++;
        } else {
            temp = true;
            break;
        }
    }
    if (temp == true) {
        print_invalid_command(buff);
        return;
    }
    //valid command
    size_t num = atoi(buff + 1);
    if (num >= vector_size(history_vector)){
        print_invalid_index();
        return;
    }

    char* exe_buff = vector_get(history_vector, num);
    //handle_history(exe_buff);
    //will be handled later
    printf("%s\n", exe_buff);
    //完成打印 需要去检查是否包含operator
    int judge = operator_execution(exe_buff);
    if (judge == -1) {
        //没有找到operator command
        buildin_execution(exe_buff);
    }
    return;
}

void prefix_distribution(char* buff) {
    //首先检查！后是不是空的
    int length = strlen(buff);
    char* exe_buff = NULL;
    char* temp = buff+1;
    if (length == 1){
        exe_buff = vector_get(history_vector, (vector_size(history_vector) - 1));
    } else {
        //从最后一位开始对比
        for (int i = vector_size(history_vector) - 1; i >= 0; i--) {
            if (!strstr(vector_get(history_vector, i), temp)) {
                continue;
            } else {
                //we find it
                exe_buff = vector_get(history_vector, i);
                break;
            }
        }
    
    }
    if (!exe_buff) {
        //没有找到comand
        print_no_history_match();
        //printf("111");
        return;
    }
    //printf("222");
    //handle_history(exe_buff);
    //history_distribution();
    printf("%s\n", exe_buff);
    int judge = operator_execution(exe_buff);
    if (judge == -1) {
        //不包含operator
        buildin_execution(exe_buff);
        //history will be handled here
    }
    return;
}

int redirection_execution(char* buff) {
    char** split_str = strsplit(buff, ' ');
    char** rotator = split_str;
    char* in_rotator = split_str[0];
    int count = 0;
    int word = 0;
    sstring* command = cstr_to_sstring(buff);
    while (*rotator) {
        while(*in_rotator) {
            word++;
            in_rotator++;
        }
        count++;
        rotator++;
    }
    if (count < 3) {
        return 0;
    }
    if (strcmp(split_str[count - 2], ">") == 0) {
        for (unsigned i = 0; i < strlen(buff); i++) {
            if (buff[i] == '>') {
                if (i + 2 >= strlen(buff)) {
                    print_invalid_command(buff);
                }
                char* redic_cmd = sstring_slice(command, 0, i - 1);
                char* redic_file = sstring_slice(command, i + 2, strlen(buff));
                /** 为了测试
                char* cmd_r = redic_cmd;
                while (*cmd_r) {
                    printf("%c", *cmd_r);
                    cmd_r++;
                }
                char* file_r = redic_file;
                while (*file_r) {
                    printf("%c", *file_r);
                    file_r++;
                }
                **/
                extend_execution(redic_cmd, redic_file, 1);
                return 1;
            }
        }
        return 0;

    } else if (strcmp(split_str[count - 2], ">>") == 0) {
        for (unsigned i = 0; i < strlen(buff); i++) {
            if (buff[i] == '>' && buff[i + 1] == '>') {
                if (i + 3 >= strlen(buff)) {
                    print_invalid_command(buff);
                }
                char* redic_cmd = sstring_slice(command, 0, i - 1);
                char* redic_file = sstring_slice(command, i + 3, strlen(buff));
                extend_execution(redic_cmd, redic_file, 2);
                return 1;
            }
        }
        return 0;
    } else if (strcmp(split_str[count - 2], "<") == 0) {
        for (unsigned i = 0; i < strlen(buff); i++) {
            if (buff[i] == '<') {
                if (i + 2 >= strlen(buff)) {
                    print_invalid_command(buff);
                }
                char* redic_cmd = sstring_slice(command, 0, i - 1);
                char* redic_file = sstring_slice(command, i + 2, strlen(buff));
                extend_execution(redic_cmd, redic_file, 3);
                return 1;
            }
        }
        return 0;
    }

    return 0;
}




int operator_execution(char*buff) {
    //只检查逻辑
    //char** new_buff = strsplit(buff, ' ');
    if (strstr(buff, " && ")) {
        and_distribution(buff);
        handle_history(buff);
        return 1;
    } else if (strstr(buff, " || ")){
        or_distribution(buff);
        handle_history(buff);
        return 1;
    } else if (strstr(buff, "; ")) {
        separator_distribution(buff);
        handle_history(buff);
        return 1;
    }
    return -1;
}




//the main function
//------------------------------------------------------

int shell(int argc, char *argv[]) {
    // TODO: This is the entry point for your shell.
    //fisrt handle the hf file 
    
    history_vector = vector_create(string_copy_constructor, string_destructor, string_default_constructor);
    //sb玩意儿让老子改了一晚上
    //process_vec = vector_create(string_copy_constructor, string_destructor, string_default_constructor);
    process_vec = shallow_vector_create();
    int _opt;
    bool judge = false;

    if (argc == 1) {
        process* initial_process = process_vec_create(argv[0], getpid());
        vector_push_back(process_vec, initial_process);
    }
    while ((_opt = getopt(argc, argv, "hf")) != -1) {
        switch(_opt){
            case 'f':
                //we should directly get the command from the file and execute it
                if (argv[optind] ==  NULL) {
                    print_usage();
                    return 0;
                } else {
                    vector* file_vec = load_f(get_full_path(argv[optind]));
                    for(size_t i = 0; i < vector_size(file_vec); i++) {
                        char* f_buff = vector_get(file_vec, i);
                        char _temp[80];
                        if (getcwd(_temp, sizeof(_temp))) {
                            print_prompt(getcwd(_temp, sizeof(_temp)), getpid());
                            printf("%s\n", f_buff);
                        }
                        int temp1 = -500;
                        temp1 = operator_execution(f_buff);
                        if (temp1 == -1) {
                            buildin_execution(f_buff);
                        }
                    }
                }
                break;
            case 'h':
                judge = true;
                if (argv[optind] ==  NULL) {
                    break;

                }
                load_in_his(get_full_path(argv[optind]));
            default:
                break;
        }
    }
    
    //first print out the current path

    
    char _print[80];
    getcwd(_print, sizeof(_print));
    print_prompt(getcwd(_print, sizeof(_print)), getpid());
    char* buff = NULL;
    size_t buff_size = 0;
    while (getline(&buff, &buff_size, stdin) != -1) {
        //continue until the invalid read
        //替换末尾
        if (buff[strlen(buff) - 1] == '\n') {
            buff[strlen(buff) - 1] = '\0';
        }
        //这是打印在terminal上的指令 开始分情况讨论
        if (strcmp(buff, "exit\0") == 0) {
            //small bug
            break;
        }
        //需要检查全部的命令
        int temp = -500;
        int temp2 = -500;
        int temp3 = -500;
        //先检查逻辑富豪
        temp = operator_execution(buff);
        if (temp == -1) {
            //不存在逻辑符号
            temp2 = redirection_execution(buff);
            if (temp2 != 1) {
                temp3 = buildin_execution(buff);
                if (temp3 != 1) {
                    //外建命令没有执行
                    print_invalid_command(buff);
                    handle_history(buff);
                }
            }

        }
        if (getcwd(_print, sizeof(_print))) {
            print_prompt(getcwd(_print, sizeof(_print)), getpid());
        }
        if (judge) {
            char* temp3 = get_full_path(argv[optind]);
            FILE* temp4 = fopen(temp3, "w");
            for (size_t i = 0; i < vector_size(history_vector); i++) {
                //printf("current index is %zu", i);
                fputs(vector_get(history_vector, i), temp4);
                fputs("\n", temp4);
            }
            fclose(temp4);
        }

    }
    
    free(buff);
    vector_destroy(history_vector);
    return 0;
}
