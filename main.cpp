#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdlib>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>
#include <algorithm>
#include <dirent.h>
#include <limits.h>
#include <linux/rtc.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <cstdlib>

using namespace std;

void pwd()//Имя текущей директории
{
    string s(get_current_dir_name());
    cout << s << '\n';
}

void cd(string path)//Процедура смены рабочей директории
{
    if (path == "")
    {
        string s = getenv("HOME");
        chdir(s.c_str());
    }
    else
    {
        if(chdir(path.c_str()) == -1)
        {
            perror("Wrong path");
        }
    }
}

int main()
{
    while(1)//Главный цикл работы программы
    {
        string cur(get_current_dir_name());
        if (geteuid() == 0)
            cout << cur <<  "~$ ";
        else
            cout << cur <<  "~> ";
        string input;
        getline(cin, input);
        if(input.size() == 0)
            continue;
        //Нарезаем строчку на вектор вектор строк сначала по | затем по пробелу
        vector<string> split;
        string delim(" | ");
        size_t prev = 0;
        size_t next;
        size_t delta = delim.length();
        while((next = input.find(delim, prev)) != string::npos )
        {
            string tmp = input.substr(prev, next-prev);
            split.push_back( input.substr(prev, next-prev));
            prev = next + delta;
        }
        string tmp = input.substr(prev);
        split.push_back(input.substr(prev));
        vector<vector <string>> supersplit;
        supersplit.resize(split.size());
        for(int i = 0; i < split.size(); i++)
        {
            string word;
            stringstream s(split[i]);
            while (s >> word) supersplit[i].push_back(word);
        }
        //Закончили нарезку

        if(supersplit[0][0] == "cd")//Уходим в другое место
        {
            if (supersplit[0].size() == 1)
            {
                cd("");
            }
            else
            {
                cd(supersplit[0][1]);
            }
        }
        else if(supersplit[0][0] == "pwd")//Где мы?
        {
            pwd();
        }
        else if(supersplit[0][0] == "echo")//Вывести введенное
        {
            for(int i = 1; i < supersplit[0].size(); i++){
                cout << supersplit[0][i] << ' ';
            }
            cout << '\n';
        }
        else if(supersplit[0][0] == "set")//настройка
        {
            //char * Ptr = getenv("LANG");
            //cout << "Lang = " << Ptr << '\n';
            extern char ** environ;
            for (int i = 0; environ[i] != NULL; i++)
            {
                cout << environ[i] << '\n';
            }
        }
        else//Запуск без конвеера
        {
            size_t num = 0;
            struct timeval start,stop;
            if(supersplit[0][0] == "time")
            {
                num = 1;
                gettimeofday(&start,NULL);
            }
            if(supersplit.size() == 1)
            {
                //Переключаем
                //if(find(supersplit[0].begin(),supersplit[0].end(), '<') != supersplit[0].end()){}
                //if(find(supersplit[0].begin(), supersplit[0].end(), '>') != supersplit[0].end()){}
                pid_t ch = fork();
                if (ch != 0)   //parent
                {
                    wait(0);
                }
                else    //children
                {
                    vector<char *> v;
                    for (size_t i = num; i < supersplit[0].size(); i++)
                    {
                        v.push_back((char *)supersplit[0][i].c_str());
                    }
                    v.push_back(NULL);
                    prctl(PR_SET_PDEATHSIG, SIGINT);
                    execvp(v[0], &v[0]);
                    perror(v[0]);
                    exit(1);
                }
            }
            else//Запуск с конвеером
            {
                int orig_stdin = dup(STDIN_FILENO); // изначальные fd
                int orig_stdout = dup(STDOUT_FILENO);
                int pipes[supersplit.size()-1][2];
                int cur_fd = 0;
                int prev_fd = -1;
                for(int i = 0; i < supersplit.size()-1; i++)
                    if(pipe2(pipes[i], O_CLOEXEC))
                        perror("pipe\n");
                for (int i = 0; i < supersplit.size(); i++)
                {
                    //if(find(supersplit[i].begin(),supersplit[i].end(), "<") != supersplit[i].end()){}
                    //if(find(supersplit[i].begin(), supersplit[i].end(), ">") != supersplit[i].end()){}
                    pid_t pid = fork();
                    if (!pid)
                    {
                        //Для первой команды нужно переключить stdout
                        if (!i)
                        {
                            dup2(pipes[0][1], STDOUT_FILENO);
                            close(pipes[0][0]);
                        }
                        //Для непервой и непоследней команды нужно переключить stdin и stdout
                        if (i && i < supersplit.size() - 1)
                        {
                            dup2(pipes[prev_fd][0], STDIN_FILENO);
                            close(pipes[prev_fd][1]);
                            dup2(pipes[cur_fd][1], STDOUT_FILENO);
                            close(pipes[cur_fd][0]);
                        }
                        //Для последней команды нужно переключить stdin
                        if(i == supersplit.size() - 1)
                        {
                            dup2(pipes[prev_fd][0], STDIN_FILENO);
                            close(pipes[prev_fd][1]);
                        }
                        //Сформировать запуск
                        vector<char *> v;
                        for (size_t j = 0; j < supersplit[i].size(); j++)
                        {
                            v.push_back((char *)supersplit[i][j].c_str());
                        }
                        v.push_back(NULL);
                        prctl(PR_SET_PDEATHSIG, SIGINT);
                        execvp(v[0], &v[0]);
                        perror(v[0]);
                        exit(1);
                    }//
                    else
                    {
                        if(i == supersplit.size() - 1)
                        {
                            for(int j = 0; j < supersplit.size() - 1; j++)
                            {
                                close(pipes[j][0]);
                                close(pipes[j][1]);
                            }
                            for(int j = 0; j < supersplit.size() - 1; j++)
                                wait(0);
                        }
                    }
                    cur_fd++;
                    prev_fd++;
                }
            dup2(orig_stdin, STDIN_FILENO);
            dup2(orig_stdout, STDOUT_FILENO);
            close(orig_stdin);
            close(orig_stdout);
            }
            if(num)//Выводим время работы дочерних процессов
            {
                //stop = time(NULL);
                //real_time = difftime(stop,start);
                gettimeofday(&stop,NULL);
                struct rusage usage;
                getrusage(RUSAGE_CHILDREN, &usage);
                cout << "\nreal     " << stop.tv_sec - start.tv_sec << '.' << (stop.tv_usec - start.tv_usec)/1000 << "s\nuser     "
                     << usage.ru_utime.tv_sec << '.' << (usage.ru_utime.tv_usec)/1000 << "s\nsystem   "
                     <<  usage.ru_stime.tv_sec << '.' << (usage.ru_stime.tv_usec)/1000 << "s\n";
            }
        }
    }
    return 0;
}