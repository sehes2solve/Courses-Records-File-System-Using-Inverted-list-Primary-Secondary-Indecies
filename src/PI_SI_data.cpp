#include "PI_SI_data.h"

PI_SI_data::PI_SI_data()
{
    //ctor
}


void PI_SI_data::print_course(Course& c)
{
    cout << "Enter Course ID : " << c.ID << endl;
    cout << "Enter Course Name : " << c.name << endl;
    cout << "Enter Course Instructor : " << c.instructor << endl;
    cout << "Enter Course Number of Weeks : " << c.weeks << endl;
}
void PI_SI_data::enter_course(Course& c)
{
    cout << "Enter Course Data : " << endl;
    cout << "Enter Course ID : ";
    cin >> c.ID;
    cin.ignore();
    cout << "Enter Course Name : ";
    getline(cin,c.name);
    cout << "Enter Course Instructor : ";
    getline(cin,c.instructor);
    cout << "Enter Course Number of Weeks : ";
    cin >> c.weeks;
}
void PI_SI_data::write_course(const Course& c,fstream& file)
{
    short len = sizeof(c.ID) + sizeof(c.weeks) + c.instructor.size() + c.name.size() + 2;/// 2 delimiter char '|'
    file.write((char*)&len,sizeof(len));
    file.write(c.ID,sizeof(c.ID));
    file.write(c.name.c_str(),c.name.size());file.put('|');
    file.write(c.instructor.c_str(),c.instructor.size());file.put('|');
    file.write((char*)&c.weeks,sizeof(c.weeks));
}
void PI_SI_data::read_course(Course& c,fstream& file)
{
    short len;
    file.read((char*)&len,sizeof(len));
    string buff; buff.resize(len);
    file.read(&buff[0],len);
    stringstream ss(buff);
    ss.read(c.ID,sizeof(c.ID));
    getline(ss,c.name,'|');
    getline(ss,c.instructor,'|');
    ss.read((char*)&c.weeks,sizeof(c.weeks));
}

bool PI_SI_data::getIstate()///called only once when about to load index file
{
    fstream data_file(data_file_name.c_str(),ios::in|ios::app);
    bool IsUpToDate;
    data_file.read((char*)&IsUpToDate,sizeof(IsUpToDate));
    if(data_file.eof())///data file was just created.
    {
        data_file.close();
        setIstate(true);
        return true;
    }
    data_file.close();
    return IsUpToDate;
}
void PI_SI_data::setIstate(bool IsUpToDate)///the data file would be created before it's called
{
    fstream data_file(data_file_name.c_str(),ios::out|ios::in);
    data_file.write((char*)&IsUpToDate,sizeof(IsUpToDate));
    data_file.close();
}
bool PI_SI_data::IsExist(char* FileName)
{
    fstream file(FileName,ios::in);
    if(file.fail())
    {
        file.close();
        return false;
    }
    else
    {
        file.close();
        return true;
    }
}
void PI_SI_data::construct_I()
{
    fstream data_file(data_file_name.c_str(),ios::in|ios::app);
    /// Note : append mode => read only from file
    bool IsUpToDate;data_file.read((char*)&IsUpToDate,HeaderSize);
    if(data_file.fail())/// the data file is empty => just created
    {
        data_file.close();
        setIstate(true);
        fstream data_file(data_file_name.c_str(),ios::in);
        data_file.seekg(HeaderSize,ios::beg);///jumps the header
    }
    loaded_PI.clear();loaded_SI.clear();
/// DR position   ;Position of SK;Crs bucket ;PI bucket    ;SI bucket-new SK;PK bucket for new PKs in a recorded SK;
    int offset = 0,SK_P_loaded_SI;Course temp;PI_record rec;SI_record SI_rec;string new_PK;new_PK.resize(6);
    while(true)
    {
        offset = data_file.tellg();
        read_course(temp,data_file);
        if(data_file.fail())
            break;
        if(temp.ID[0] == '*')
            continue;
        /// primary index
        strcpy(rec.PK,temp.ID);
        rec.data_offset = offset;
        loaded_PI.push_back(rec);
        /// secondary index
        SK_P_loaded_SI = BinarySearch_SI(temp.instructor);
        if(SK_P_loaded_SI != -1)///the SK was recorded before with different PK
        {
            strcpy(&new_PK[0],temp.ID);
            loaded_SI[SK_P_loaded_SI].PK_strings.push_back(new_PK);
        }
        else
        {
            SI_rec.SK =  temp.instructor;
            strcpy(&new_PK[0],temp.ID);
            SI_rec.PK_strings.push_back(new_PK);
            loaded_SI.push_back(SI_rec);
        }
        SI_rec.PK_strings.clear();///clears the PKs before pushing new PKs of other SK
    }
    data_file.close();
    sort_loaded_PI();
    sort_loaded_SI();
}
void PI_SI_data::load_I()
{
    if(!(IsExist(&PI_file_name[0]) && IsExist(&SI_file_name[0]) && IsExist(&IL_file_name[0]) && IsExist(&data_file_name[0])))
    /// Checks whether any file was missing
    {
        ofstream PI_file(PI_file_name.c_str()),SI_file(SI_file_name.c_str()),IL_file(IL_file_name.c_str());
        ///intialize primary,secondary(including I.L) index files
        construct_I();///=> intialize data file
        PI_file.close();SI_file.close();IL_file.close();
    }
    else if(getIstate())/// Checks if data is updated since all files are there
    {
        PI_record temp;loaded_PI.clear();
        ifstream PI_file(PI_file_name.c_str()),SI_file(SI_file_name.c_str()),IL_file(IL_file_name.c_str());
        while(true)
        {
            PI_file.read((char*)&temp,sizeof(temp));
            if(PI_file.fail())
                break;
            loaded_PI.push_back(temp);
        }
        ///read from SI and IL file
        SI_record SI_temp;SI_record::IL_record IL_temp;string PK_temp;PK_temp.resize(6);
        while(true)
        {
            getline(SI_file,SI_temp.SK,'|');
            if(SI_file.fail())
                break;
            SI_file.read((char*)&IL_temp.next_PK,sizeof(IL_temp.next_PK));
            while(IL_temp.next_PK != -1)///this loop must be entered at least as there must be at least one PK
            {
                IL_file.seekg(IL_temp.next_PK,ios::beg);
                IL_file.read((char*)&IL_temp,sizeof(IL_temp));
                strcpy(&PK_temp[0],IL_temp.PK);
                SI_temp.PK_strings.push_back(PK_temp);
            }
            loaded_SI.push_back(SI_temp);
            SI_temp.PK_strings.clear();///clears the PKs before pushing new PKs of other SK
        }
        PI_file.close();SI_file.close();IL_file.close();
    }
    else/// all files are found but data isn't updated
    {
        ofstream PI_file(PI_file_name.c_str()),SI_file(SI_file_name.c_str()),IL_file(IL_file_name.c_str());
        ///Deletes old indices
        PI_file.close();SI_file.close();IL_file.close();
        construct_I();
    }
    /*cout << "Primary Keys : " << endl;
    for(int i = 0;i < loaded_PI.size();i++)
        cout << "PI : " << i << " " << loaded_PI[i].PK << endl;

    cout << "Sec keys : " << endl;
    for(int i = 0;i < loaded_SI.size();i++)
    {
        cout << "SI : " << i << " " << loaded_SI[i].SK << endl;
        for(int j = 0;j < loaded_SI[i].PK_strings.size();j++)
        {
            cout << loaded_SI[i].PK_strings[j] << endl;
        }
    }*/
}
void PI_SI_data::save_I()
{
    ofstream PI_file(PI_file_name.c_str()),SI_file(SI_file_name.c_str()),IL_file(IL_file_name.c_str());
    SI_record::IL_record IL_temp;int PKs_size;
    for(unsigned int i = 0;i < loaded_PI.size();i++)
        PI_file.write((char*)&loaded_PI[i],sizeof(PI_record));

    for(unsigned int i = 0;i < loaded_SI.size();i++)
    {
        ///Secondary Index file
        IL_temp.next_PK = IL_file.tellp();
        SI_file.write(&loaded_SI[i].SK[0],loaded_SI[i].SK.size());SI_file.put('|');///SK is written in delimited field
        SI_file.write((char*)&IL_temp.next_PK,sizeof(IL_temp.next_PK));///writes the position of PK in the IList file
        ///Inverted List file
        PKs_size = loaded_SI[i].PK_strings.size();
        for(int j = 0;j < PKs_size - 1;j++)
        {
            IL_temp.next_PK = (short)IL_file.tellp() +  sizeof(IL_temp);
            strcpy(IL_temp.PK,&loaded_SI[i].PK_strings[j][0]);
            IL_file.write((char*)&IL_temp,sizeof(IL_temp));
        }
        IL_temp.next_PK = -1;strcpy(IL_temp.PK,&loaded_SI[i].PK_strings[PKs_size - 1][0]);
        IL_file.write((char*)&IL_temp,sizeof(IL_temp));
    }
    PI_file.close();SI_file.close();IL_file.close();
    setIstate(true);
}

void PI_SI_data::sort_loaded_PI()
{
    /*for(int gap = loaded_PI.size();gap > 0;gap /= 2)
    {
        for(int i = gap;i < loaded_PI.size();i++)
        {
            int j;
            PI_record temp = loaded_PI[i];
            for(j = i;atoi(loaded_PI[j - gap].PK) > atoi(temp.PK) && j > gap; j -= gap)
                loaded_PI[j] = loaded_PI[j-gap];
            if(j == gap)
            {   if(atoi(loaded_PI[j - gap].PK) > atoi(temp.PK))
                {
                    loaded_PI[j] = loaded_PI[j-gap];
                    j--;
                }
            }
            loaded_PI[j] = temp;
        }
    }*/
    for(unsigned int i = 1;i < loaded_PI.size();i++)
    {
        int least = i-1;
        for(unsigned int j = i;j < loaded_PI.size();j++)
        {
            if(atoi(loaded_PI[j].PK) < atoi(loaded_PI[least].PK))
                swap(loaded_PI[j],loaded_PI[least]);
        }
    }
}
void PI_SI_data::sort_loaded_SI()
{
    /*for(unsigned int gap = loaded_SI.size()/2;gap > 0;gap /= 2)
    {
        for(unsigned int i = gap;i < loaded_SI.size();i++)
        {
            int j = i;
                SI_record temp = loaded_SI[i];
                for(;loaded_SI[j - gap].SK > temp.SK && j > gap; j -= gap)
                    loaded_SI[j] = loaded_SI[j-gap];
                if(j == gap)
                {
                    if(loaded_SI[j - gap].SK > temp.SK)
                    {
                        loaded_SI[j] = loaded_SI[j-gap];
                        j--;
                    }
                }
                loaded_SI[j] = temp;

        }
    }*/
    for(unsigned int i = 1;i < loaded_SI.size();i++)
    {
        int least = i-1;
        for(unsigned int j = i;j < loaded_SI.size();j++)
        {
            if(loaded_SI[j].SK < loaded_SI[least].SK)
                swap(loaded_SI[j],loaded_SI[least]);
        }
    }
}
void PI_SI_data::sort_strings(vector<string>& s)
{
    /*for(int gap = s.size()/2;gap > 0;gap /= 2)
    {
        for(unsigned int i = gap;i < s.size();i++)
        {
            int j;
            string temp = s[i];
            for(j = i;s[j - gap] > temp && j > gap; j -= gap)
                s[j] = s[j-gap];
            if(j == gap)
            {
                if(s[j - gap] > temp)
                {
                    s[j] = s[j-gap];
                    j--;
                }
            }
            s[j] = temp;
        }
    }*/
    for(unsigned int i = 1;i < s.size();i++)
    {
        int least = i-1;
        for(unsigned int j = i;j < s.size();j++)
        {
            if(s[j] < s[least])
                swap(s[j],s[least]);
        }
    }
}

int PI_SI_data::BinarySearch_PI(char* PK)
{
    int low = 0,high = loaded_PI.size() - 1,mid;
    while(low <= high)
    {
        mid = (low + high)/2;
        if(!strcmp(loaded_PI[mid].PK,PK))
            return mid;
        else if(atoi(loaded_PI[mid].PK) > atoi(PK))
            high = mid - 1;
        else
            low = mid + 1;
    }
    return -1;
}
int PI_SI_data::BinarySearch_SI(string SK)
{
    int low = 0,high = loaded_SI.size() - 1,mid;
    while(low <= high)
    {
        mid = (low + high)/2;
        if(loaded_SI[mid].SK == SK)
            return mid;
        else if(loaded_SI[mid].SK > SK)
            high = mid - 1;
        else
            low = mid + 1;
    }
    return -1;
}
int PI_SI_data::BinarySearch_strings(vector<string>& strings,string s)
{
    int low = 0,high = strings.size() - 1,mid;
    while(low <= high)
    {
        mid = (low + high)/2;
        if(strings[mid] == s)
            return mid;
        else if(strings[mid] > s)
            high = mid - 1;
        else
            low = mid + 1;
    }
    return -1;
}


void PI_SI_data::add_replace_PI(const Course& temp_c,fstream& data_file,int PI_ix)
{
    PI_record temp_i;
    temp_i.data_offset = data_file.tellp();
    strcpy(temp_i.PK,temp_c.ID);
    if(PI_ix == -1)
        loaded_PI.push_back(temp_i);
    else
        loaded_PI[PI_ix] = temp_i;
    sort_loaded_PI();
}
void PI_SI_data::add_PK_to_SI(const Course& temp_c)
{
    string temp_PK = temp_c.ID;
    int SI_pos = BinarySearch_SI(temp_c.instructor);
    if(SI_pos == -1)
    {
        SI_record temp_si;temp_si.SK = temp_c.instructor;
        temp_si.PK_strings.push_back(temp_PK);
        loaded_SI.push_back(temp_si);
        sort_loaded_SI();
    }
    else
    {
        loaded_SI[SI_pos].PK_strings.push_back(temp_PK);
        sort_strings(loaded_SI[SI_pos].PK_strings);
    }
}
void PI_SI_data::remove_PI(int ix)
{
    for(unsigned int j = ix;j < loaded_PI.size() - 1 ;j++)
        loaded_PI[j] = loaded_PI[j+1];
    loaded_PI.pop_back();
}
void PI_SI_data::remove_SI(int ix)
{
    for(unsigned int j = ix;j < loaded_SI.size() - 1 ;j++)
        loaded_SI[j] = loaded_SI[j+1];
    loaded_SI.pop_back();
}
void PI_SI_data::remove_PK_of_SI(string SK,char PK[6])
{
    int ix = BinarySearch_SI(SK);
    if(ix == -1)
    {
        cout << "Course of Instructor (" << SK << ") isn't found\n";
        return;
    }
    else
    {
        for(unsigned int i = 0;i < loaded_SI[ix].PK_strings.size() ;i++)
        {
            if(strcmp(loaded_SI[ix].PK_strings[i].c_str(),PK) == 0)
            {
                for(unsigned int j = i;j < loaded_SI[ix].PK_strings.size() - 1 ;j++)
                    loaded_SI[ix].PK_strings[j] = loaded_SI[ix].PK_strings[j+1];
                loaded_SI[ix].PK_strings.pop_back();
                if(loaded_SI[ix].PK_strings.empty())
                    remove_SI(ix);
                return;
            }
        }
        cout << "Course of Instructor (" << SK << ") & ID (" << PK << ") isn't found\n";
    }
}


void PI_SI_data::add_course()
{
    Course temp_c;
    enter_course(temp_c);
    setIstate(false);
    fstream data_file(data_file_name.c_str(),ios::out|ios::app);
    data_file.clear();data_file.seekp(0,ios::end);
    ///Primary Index
    add_replace_PI(temp_c,data_file,-1);
    ///write data
    write_course(temp_c,data_file);
    ///Secondary Index
    add_PK_to_SI(temp_c);
    data_file.close();
}
void PI_SI_data::update_course_ByID(char PK[6])
{
    int PI_record_ix = BinarySearch_PI(PK);
    if(PI_record_ix == -1)
    {
        cout << "Course of ID (" << PK << ") isn't found\n" ;
        return;
    }
    else
    {
        setIstate(false);
        fstream data_file(data_file_name.c_str(),ios::out|ios::in);
        ///SK of the Course that will be updated
        Course temp_c;string SK;int removed_rec_offset = loaded_PI[PI_record_ix].data_offset;
        enter_course(temp_c);
        data_file.clear();
        data_file.seekp(0,ios::end);
        ///Primary Index
        add_replace_PI(temp_c,data_file,PI_record_ix);
        ///write data
        write_course(temp_c,data_file);///save PI
        data_file.seekg(removed_rec_offset + 2,ios::beg);///2 bits for size of Record(short)
        data_file.put('*');
        data_file.seekg(sizeof(PK)-1,ios::cur);
        getline(data_file,SK,'|');///jumps over Course Name
        getline(data_file,SK,'|');
        ///Secondary Index
        remove_PK_of_SI(SK,PK);
        add_PK_to_SI(temp_c);
        data_file.close();
    }
}
void PI_SI_data::update_course_ByInstructor(string instructor)
{
    int SI_record_ix = BinarySearch_SI(instructor);
    if(SI_record_ix == -1)
    {
        cout << "Course of Instructor (" << instructor << ") isn't found\n" ;
        return;
    }
    else
    {
        setIstate(false);
        delete_course_ByInstructor(instructor);
        add_course();
    }
}
void PI_SI_data::delete_course_ByID(char PK[6])
{
    int PI_record_ix = BinarySearch_PI(PK);
    if(PI_record_ix == -1)
    {
        cout << "Course of ID (" << PK << ") isn't found\n" ;
        return;
    }
    else
    {
        setIstate(false);
        fstream data_file(data_file_name.c_str(),ios::out|ios::in);string SK;///SK of the Course that will be updated
        data_file.clear();
        data_file.seekp(loaded_PI[PI_record_ix].data_offset + 2,ios::beg);///2 bits for size of Record(short)
        data_file.put('*');
        data_file.seekg(sizeof(PK)-1,ios::cur);
        getline(data_file,SK,'|');///jumps over Course Name
        getline(data_file,SK,'|');
        data_file.close();
        remove_PI(PI_record_ix);
        remove_PK_of_SI(SK,PK);
    }
}
void PI_SI_data::delete_course_ByInstructor(string instructor)
{
    int SI_record_ix = BinarySearch_SI(instructor);
    if(SI_record_ix == -1)
    {
        cout << "Course of Instructor (" << instructor << ") isn't found\n" ;
        return;
    }
    else
    {
        setIstate(false);int PI_record_ix;
        fstream data_file(data_file_name.c_str(),ios::out|ios::in);
        for(unsigned int i = 0;i < loaded_SI[SI_record_ix].PK_strings.size();i++)
        {
            PI_record_ix = BinarySearch_PI(&loaded_SI[SI_record_ix].PK_strings[i][0]);
            if(PI_record_ix == -1)
            {
                cout << "Course of ID (" << loaded_SI[SI_record_ix].PK_strings[i] << ") isn't found\n";
                continue;
            }
            else
            {
                data_file.clear();
                data_file.seekp(loaded_PI[PI_record_ix].data_offset + 2,ios::beg);///2 bits for size of Record(short)
                data_file.put('*');
                remove_PI(PI_record_ix);
            }
        }
        remove_SI(SI_record_ix);
        data_file.close();
    }
}
void PI_SI_data::print_by_PK(char* PK)
{
    int PI_record_ix = BinarySearch_PI(PK);
    if(PI_record_ix == -1)
    {
        cout << "Course of ID (" << PK << ") isn't found\n" ;
        return;
    }
    else
    {
        Course temp;
        fstream data_file(data_file_name.c_str(),ios::in);
        data_file.clear();
        data_file.seekg(loaded_PI[PI_record_ix].data_offset,ios::beg);
        read_course(temp,data_file);
        cout << "Course Data :\n" ;
        print_course(temp);
        data_file.close();
    }
}
void PI_SI_data::print_by_SK(string SK)
{
    int SI_record_ix = BinarySearch_SI(SK);
    if(SI_record_ix == -1)
    {
        cout << "Course of Instructor (" << SK << ") isn't found\n" ;
        return;
    }
    else
    {
        for(unsigned int i = 0;i < loaded_SI[SI_record_ix].PK_strings.size();i++)
        {
            print_by_PK(&loaded_SI[SI_record_ix].PK_strings[i][0]);
            cout << endl;
        }
    }
}


PI_SI_data::~PI_SI_data()
{
    //dtor
}
