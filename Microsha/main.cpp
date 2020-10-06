#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;

void pwd()
{
    string s(get_current_dir_name());
    cout << s << '\n';
}

void cd(string path)
{
    if (path == "0")
    {
        string s = "/home/timur";
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

void time()
{
}

int main()
{
    while(1)
    {
        string cur(get_current_dir_name());
        cout << cur <<  " > ";
        string input;
        getline(cin, input);
        vector <string> split;
        string word;
        stringstream s(input);
        while (s >> word) split.push_back(word);
        /*for(auto i = 0; i < split.size(); i++){
            cout << split[i] << '\n';
        }*/
        if(split[0] == "cd")
        {
            if (split.size() == 1)
            {
                cd("0");
            }
            else
            {
                cd(split[1]);
            }
        }
        else if(split[0] == "pwd")
        {
            pwd();
        }
        else if(split[0] == "time")
        {
            //time(...);
        }
        else
        {
            //execvp
            if (fork() != 0)   //parent
            {
                wait(0);
            }
            else   //child
            {
                vector<char *> v;
                for (size_t i = 0; i < split.size(); i++)
                {
                    v.push_back((char *)split[i].c_str());
                }
                v.push_back(NULL);
                execvp(v[0], &v[0]);
                perror(v[0]);
            }
        }

    }
    return 0;
}
