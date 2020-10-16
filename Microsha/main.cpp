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
        vector <string> split;
        string word;
        stringstream s(input);
        while (s >> word) split.push_back(word);
        if(split[0] == "cd")//Уходим в другое место
        {
            if (split.size() == 1)
            {
                cd("");
            }
            else
            {
                cd(split[1]);
            }
        }
        else if(split[0] == "pwd")//Где мы?
        {
            pwd();
        }
        else
        {
            size_t num = 0;
            unsigned long long real_time;
            if(split[0] == "time"){
                num = 1;
                real_time =  clock();
            }
            if (fork() != 0)   //parent
            {
                wait(0);
            }
            else   //children
            {
                vector<char *> v;
                for (size_t i = num; i < split.size(); i++)
                {
                    v.push_back((char *)split[i].c_str());
                }
                v.push_back(NULL);
                execvp(v[0], &v[0]);
                perror(v[0]);
            }
            if(num)//Выводим время работы дочерних процессов
            {
                real_time = clock() - real_time;
                struct rusage usage;
                getrusage(RUSAGE_CHILDREN, &usage);
                cout << "CPU time:\n" << ((double) real_time)/1000 << " sec real\n"
                << usage.ru_utime.tv_sec << '.' << usage.ru_utime.tv_usec << " sec user\n"
                <<  usage.ru_stime.tv_sec << '.' << usage.ru_stime.tv_usec << " sec system\n";
            }
        }
    }
    return 0;
}
