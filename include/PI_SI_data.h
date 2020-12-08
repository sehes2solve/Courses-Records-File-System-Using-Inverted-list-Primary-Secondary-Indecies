#ifndef PI_SI_DATA_H
#define PI_SI_DATA_H
#include<iostream>
#include<string>
#include<cstring>
#include<vector>
#include<algorithm>
#include<bits/stdc++.h>

using namespace std;

class PI_SI_data
{
    private:

        struct Course
        {
            char ID[6];
            string name, instructor;
            short weeks;
        };

        struct PI_record/// or inverted list record as it's consists of PK and offset of following PK of same SK in Inverted list file
        {
            char PK[6];
            int data_offset;
        };

        struct SI_record
        {
            string SK;
            vector<string> PK_strings;
            struct IL_record
            {
                char PK[6] ;
                short next_PK;
            };
        };
        int HeaderSize = sizeof(bool);
        string data_file_name = "data.txt",PI_file_name = "PI.txt",SI_file_name = "SI.txt" , IL_file_name = "IL.txt";
        vector<PI_record> loaded_PI;
        vector<SI_record> loaded_SI;

        void print_course(Course&);
        void enter_course(Course&);
        void write_course(const Course&,fstream&);
        void read_course(Course&,fstream&);

        bool getIstate();///Also intialize data file incase index file was created and data file wasn't
        void setIstate(bool);
        bool IsExist(char*);///Check index file
        void construct_I();///Also intialize data file

        void sort_loaded_PI();
        void sort_loaded_SI();
        void sort_strings(vector<string>&);
        int BinarySearch_PI(char*);
        int BinarySearch_SI(string);
        int BinarySearch_strings(vector<string>&,string);


        void add_replace_PI(const Course&,fstream&,int);///sets offset field of new PI by the current putter position
        void add_PK_to_SI(const Course&);
        void remove_PI(int);
        void remove_SI(int);
        void remove_PK_of_SI(string,char[6]);

    protected:

    public:

        PI_SI_data();
        void load_I();///intialize system => all files
        void save_I();
        void add_course();
        void update_course_ByID(char[6]);
        void update_course_ByInstructor(string);
        void delete_course_ByID(char[6]);
        void delete_course_ByInstructor(string);
        void print_by_PK(char*);
        void print_by_SK(string);
        virtual ~PI_SI_data();
};

#endif // PI_SI_DATA_H
