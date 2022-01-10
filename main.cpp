#include <bits/stdc++.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <termios.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <ctime>
#include <iostream>
#include <iomanip>

using namespace std;


/******************** FUNCTION DECLARATION *******************************************/
void moveCursor(int x, int y);
void printError(string s);
void initialize();
void refreshScreen();
void processCurrDirectory(char *);



/********************************* VARIABLES *****************************************/

int error =0;
char cwd[4096];
int cwdSize = 4096;
string appStartPath,homePath;


vector<string> fileNames;
stack<string> leftMov;
stack<string> rightMov;
vector<string> commandsIn;

enum keyMovements
{
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN
};

struct config
{
    // to keep track of cursor's x and y position
    int cx, cy;
    int top, bottom, hCenter, bottomMax,commandCursor,commandCursorY;
    int screen_rows;
    int screen_cols;
    struct termios orig_termios;
};

config E;

/********************* GET HOME DIRECTORY ************************************/
// function to get Home Dir
char* getHomeDir(){
    /*getuid to get the user id of the current user and then getpwuid to get the password entry which includes the home directory of the user */ 
    struct passwd *pw = getpwuid(getuid());
    char *homedir = pw->pw_dir;
    return homedir;
}


// Function to disable Normal Mode and switch to command Mode

void disableNormalMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        printError("tcseattr");
}

// void window_resize_handler(int signal){
//     initialize();
    
//     refreshScreen();
//     processCurrDirectory(cwd);
// }

// Enable Raw Mode  initialize

void enableNormalMode()
{
    // getting original attributes
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
    {
        printError("tcseattr");
    }
    atexit(disableNormalMode);

    //copying it to another variable  
    struct termios raw = E.orig_termios;

    // signal(SIGWINCH, window_resize_handler);
    //changing  required flags
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); //Ctrl S and Q
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); //ISIG : Ctrl C and Z   IEXTEN: Ctrl V
    //     raw.c_cc[VMIN] = 0;
    //   raw.c_cc[VTIME] = 10;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}


/*************************** WINDOW SIZE ***************************************************/

int getCursorPosition(int *rows, int *cols){

    char buff[32];
    unsigned int i =0;
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
    while(i<sizeof(buff)-1){
        if(read(STDIN_FILENO, &buff[i], 1) != 1)
            break;
        if(buff[i]=='R')
            break;
        i+=1;
    }
    buff[i]='\0';
    if(buff[0] != '\x1b' || buff[1]!= '[')
        return -1;
    if(sscanf(&buff[2], "%d;%d", rows,cols) !=2)
        return -1;
    
    return 0;
}

// function to get window size
int getWindowSize(int *rows, int *cols){
    struct winsize ws;

    if(ioctl(STDOUT_FILENO,TIOCGWINSZ, &ws) == -1 || ws.ws_col ==0){
            if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) 
                return -1;
            return getCursorPosition(rows, cols);
        
    }
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
}

/*************************** REFRESH SCREN  ******************************************/
//function to refreshScreeen
void refreshScreen()
{
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    E.cx=0;
    E.cy=0;
}

void cursorTop(){
    write(STDOUT_FILENO, "\x1b[H", 3);
    E.cx=0;
    E.cy=0;
}

/*********************************** CURSOR MOVEMENT ********************************************/
void moveCursor(int x, int y)
{
    char buf[120];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", y+1, x);
    write(STDOUT_FILENO, buf, strlen(buf));
}

void posCursor(int key)
{
    switch (key)
    {
    case ARROW_UP:
        if(E.cy>0)
            E.cy--;
        break;
    case ARROW_DOWN:
        if(E.cy+E.top<E.bottom-1)
            E.cy++;
        break;
    }
}

/******************* PROCESSING & DISPLAY OF DIRECTORY DATA *******************/
void display(){
    // Variables to determine the file owner & group
    struct passwd *tf; 
    struct group *gf;

    //fileStat: It's how we'll retrieve the stats associated to the file. 
    struct stat fileStat;
    const char* fName;
    int fSize = fileNames.size();
    for(int itr=E.top;itr<min(E.bottom,fSize);itr++){
        fName = fileNames[itr].c_str();  //c_str() to convert string to lstat
        lstat(fName,&fileStat);

        //Directory or not
        cout<<((S_ISDIR(fileStat.st_mode)) ? "d" : "-");
        //Persmissions
        cout<<( (fileStat.st_mode & S_IRUSR) ? "r" : "-");
        cout<<( (fileStat.st_mode & S_IWUSR) ? "w" : "-");
        cout<<( (fileStat.st_mode & S_IXUSR) ? "x" : "-");
        cout<<( (fileStat.st_mode & S_IRGRP) ? "r" : "-");
        cout<<( (fileStat.st_mode & S_IWGRP) ? "w" : "-");
        cout<<( (fileStat.st_mode & S_IXGRP) ? "x" : "-");
        cout<<( (fileStat.st_mode & S_IROTH) ? "r" : "-");
        cout<<( (fileStat.st_mode & S_IWOTH) ? "w" : "-")<<"\t";

        // Printing Size
        double fileS= fileStat.st_size;
        int fSize;
        if (fileS >= (1 << 30)){
            fSize = round(fileS/(1 << 30));
            printf("%*dGB\t",3,fSize);
        }
        else if (fileS >= (1 << 20)){
            fSize = round(fileS/(1 << 20));
            printf("%*dMB\t",3,fSize);
        }
        else if (fileS >= (1 << 10)){
            fSize = round(fileS/(1 << 10));
            printf("%*dKB\t",3,fSize);
        }
        else{
            fSize = round(fSize);
            printf(" %*dB\t",3,fSize);
        }

        //[Owner]
        tf = getpwuid(fileStat.st_uid);
        printf("%*s\t", 15,tf->pw_name);

        //[group]
        gf = getgrgid(fileStat.st_gid);
        printf("%*s\t\t",15,gf->gr_name);

        //Modified Time
        string dtime = ctime(&fileStat.st_mtime);
        dtime[dtime.size()-1]='\0';
        // printf("\t");
        for (unsigned int i = 0; i < dtime.size(); i++)
            printf("%c", dtime[i]);
        
        printf("\t");

        //File name
        if(S_ISDIR(fileStat.st_mode)){
            cout<<"\x1b[1;34m";
            printf("%s",fName);
            cout<<"\x1b[0m"<<"/";
        }
        else{
            printf("%s",fName);
        }
        cout<<"\n\r";

    }
    moveCursor(E.hCenter,E.screen_rows);
    write(STDOUT_FILENO,"Normal Mode",11);
    moveCursor(0,E.bottomMax+2);
    string currDir = string(cwd);
    currDir = "Current Directory: " + currDir;
    write(STDOUT_FILENO,currDir.c_str(),strlen(currDir.c_str()));
    write(STDOUT_FILENO, "\x1b[H", 3);
    cursorTop();
}


void processCurrDirectory(char *dir){
    // directory: it's the folder we're browsing
    DIR* directory;

    // file: when a file is found in the directory readdir loop
    struct dirent *file;

    if((directory = opendir(dir))== NULL){
        printError("Unable to Open Directory!!");
        return;
    }
    chdir(dir);
    getcwd(cwd,cwdSize);
    fileNames.clear();

    while((file = readdir(directory))!= NULL){
        fileNames.push_back(file->d_name);
    }

    closedir(directory);
    sort(fileNames.begin(),fileNames.end());
    E.cx=0;
    E.cy=0;
    E.top=0;
    int fileSize = fileNames.size();
    E.bottom = min(E.top+E.bottomMax,fileSize);
    refreshScreen();
    display();
}

/*********************** NAVIGATION INTO FOLDER ************************************************/
//function to get parent working dir
string parent_work_dir(string path){
    path = path.substr(0, path.find_last_of("/"));
    return path;
}

/*********************** GO Back to Prev Directory **********************************************************/
void goBackOneLevel(){
    string currDir= string(cwd);
    if(cwd==appStartPath){
        return;
    }
    leftMov.push(currDir);
    // while(!rightMov.empty()){
    //     rightMov.pop();
    // }
    string parentPath = parent_work_dir(currDir);
    char *dir = &parentPath[0];
    processCurrDirectory(dir);
    return;
}


/*************** Function to go to parent DIR ***************************************************/
void parentDir(){
    string currDir= string(cwd);
    if(cwd==appStartPath){
        return;
    }
    leftMov.push(currDir);
    string parentPath = parent_work_dir(currDir);
    char *dir = &parentPath[0];
    processCurrDirectory(dir);
    return;
}

// when enter key is pressed
void onEnter(){
    struct stat fileStat;
    string fName = fileNames[E.cy+E.top];
    const char *fileName = fName.c_str();
    lstat(fileName, &fileStat);

    if(S_ISDIR(fileStat.st_mode)){

        if(strcmp(fileName,".")==0)
            return;
        else if(strcmp(fileName,"..")==0){
            parentDir();
            return;
        }
        else{
            leftMov.push(string(cwd));
            string dirPr = string(cwd) + "/" + fName;
            char *dir = &dirPr[0];
            processCurrDirectory(dir);
        }
    }
    else{
        pid_t pid = fork();
        if (pid == 0) {
        // child process
       if(execlp("/usr/bin/xdg-open","xdg-open",fileName,NULL)==-1){ printError("Cannot Open File"); return;}
        exit(1);
        
        }
    }
    return;
}

/********************** to move forward in next directory ****************************/
void moveForward(){
    if(rightMov.size() == 0)
        return;
    
    string nDir = rightMov.top();
    string currDir = string(cwd);
    rightMov.pop();
    leftMov.push(currDir);
    char *dir = &nDir[0];
    processCurrDirectory(dir);
    return;
}

/****************************** to move backward in directory **************************/
void moveBackward(){
    if(leftMov.size()==0)
        return;
    string nDir = leftMov.top();
    string currDir = string(cwd);
    leftMov.pop();
    rightMov.push(currDir);
    char *dir = &nDir[0];
    processCurrDirectory(dir);
    return;
}

/********************************* HOME ***************************************/
void home(){
    string currDir = string(cwd);
    if(appStartPath==currDir)
        return;
    leftMov.push(currDir);
    char *dir = &appStartPath[0];
    processCurrDirectory(dir);
    return;
}

/********************* SCROLL UP AND DOWN ***************************************/
void scrollUp(){
    E.top = max(E.top-1,0);
    E.bottom = E.top + E.bottomMax;
    int posy = E.cy;
    refreshScreen();
    display();
    E.cy = posy;
    moveCursor(0,posy);
}

void scrollDown(){
    int fSize = fileNames.size();
    E.bottom = min(E.bottom+1,fSize);
    E.top = (E.bottom - E.bottomMax)>0?E.bottom - E.bottomMax:0;
    int posy = E.cy;
    refreshScreen();
    display();
    E.cy = posy;
    moveCursor(0,posy);

}
/***************************** GET ABSOLUTE PATH *********************************************/
//function to create absolute path
string getAbsolutePath(string path){
    string absPath = "";
    if(path[0]=='~'){
       // cout<<"Here 1"<<endl;
        absPath = appStartPath + path.substr(1);
    }
    else if(path[0]== '.' && path[1]=='.'){
       // cout<<"Here 2"<<endl;
        string cwdir = string(cwd);
        absPath = parent_work_dir(cwdir) + path.substr(2);
    }
    else if(path[0]=='.'){
       // cout<<"Here 3"<<endl;
        absPath = string(cwd) + path.substr(1);
    }

    else if(path[0]=='/'){
        absPath = path;
    }
    else{
       // cout<<"Here 5"<<endl;
        absPath = string(cwd) + "/" + path;
    }

    return absPath;
}

/********************************** SEARCH ******************************************************/
bool search(string currDir, string toSearch){
    DIR *di;
    struct dirent *dir;
    bool find;
    string fname ;
    di=opendir(currDir.c_str());
    if(di){
        struct stat fileStat;
        while((dir = readdir(di))!= NULL){
            string dname = string(dir->d_name);
            if(dname=="..")
                continue;
            if(dname==".")
                fname = currDir;
            else
                fname = currDir + "/" + dname;
            if(fname == toSearch)
                return true;
            lstat(fname.c_str(), &fileStat);
            if(S_ISDIR(fileStat.st_mode)){
                if(dname == ".")
                    continue;
                find = search(fname,toSearch);
                if(find)
                    return true;
            }
        }
    }
    else{
        printError("Unable to Open Directory!!");
        return false;
    }
    closedir(di);
    return false;
}

/************************************ GOTO ******************************************************/
void gotoFunc(string dest){;
    leftMov.push(cwd);
    char *des = &dest[0];
   processCurrDirectory(des);
    // strcpy(cwd,des);
    write(STDOUT_FILENO, "\x1b[2J", 4);
    if(error==0){
            moveCursor(0,E.commandCursorY-5);
            string sucmgs="Go To Performed!!";
            write(STDOUT_FILENO, &sucmgs[0] ,sucmgs.size());
    }
    else{
        printError("Invalid Path");
    }
    moveCursor(E.hCenter,E.screen_rows);
    write(STDOUT_FILENO,"Command Mode",12);
    moveCursor(0,E.commandCursorY);
    return;
}

/*******************************CREATE DIRECTORY ***********************************************/
void create_dir(){
    int len = commandsIn.size();
    string des = getAbsolutePath(commandsIn[len-1]);
    int i = 1;
    string fname;
    while(i<len-1){
        fname = commandsIn[i];
        fname = des + "/" + fname;
        int fd = mkdir(fname.c_str(),0755);
        if(fd==-1)
            printError("Unable to create file!!");
        i+=1;
    }
    return;
}

/******************************* CREATE FILE **************************************************/
void create_file(){
    int len = commandsIn.size();
    string des = getAbsolutePath(commandsIn[len-1]);
    int i = 1;
    string fname;
    while(i<len-1){
        fname = commandsIn[i];
        fname = des + "/" + fname;
        int fd = open(fname.c_str(),O_RDWR | O_CREAT, 0755);
        if(fd==-1)
            printError("Unable to create file!!");
        i+=1;
    }
    return;
}

/***************************** DELETE DIRECTORY **********************************************/
void delete_directory(string dirToDel){
    DIR *di;
    int status;
    struct dirent *dir;
    if((di = opendir(dirToDel.c_str()))==NULL){
        printError("Cannot Delete Directory");
        return;
    }

    struct stat fileStat;
    while((dir=readdir(di))){
        string fileName = string(dir->d_name);
        string finalF = dirToDel+"/"+ fileName;
        lstat(finalF.c_str(),&fileStat);
        

        if(S_ISDIR(fileStat.st_mode)){
            if((fileName==".") || (fileName==".."))
                continue;
            delete_directory(dirToDel+"/"+fileName);
            rmdir(finalF.c_str());
        }
        else{
            int status = unlink(finalF.c_str());
            if(status!=0)
                printError("Unable to delete file!!");
        }

    }
    closedir(di);

}

/****************************DELETE FILE ***************************************************/
bool delete_file(string des){
    int status;
    status = unlink(des.c_str());
    if(status==0)
        return true;
    return false;
}


/*************************** COPY PROCESSING *************************************************/

//File copying function
void copy_File(string src,string dest){
    char ch;
    FILE *srcFile_f, *destFile_f;
    struct stat fromFile_stat;
    // cout<<"destination: "<<dest<<" Source "<<src;
    srcFile_f = fopen(src.c_str(), "r");
    destFile_f = fopen(dest.c_str(), "w");
    if (srcFile_f == NULL ) {
        printError("Unable to open Source File");
        return;
    }
    if(destFile_f == NULL){
        printError("Unable to open Destination File");
        return;
    }
    while ((ch = getc(srcFile_f)) != EOF) {
        putc(ch, destFile_f);
    }

    stat(src.c_str(), &fromFile_stat);
    chown(dest.c_str(), fromFile_stat.st_uid, fromFile_stat.st_gid);
    chmod(dest.c_str(), fromFile_stat.st_mode);
    fclose(destFile_f);
    fclose(srcFile_f);
    return;
}


//copying directory function
void copy_dir_files(string source,string dest){
    struct dirent *dir;
    DIR *di;
    // cout<<"Source: "<<source<<" Destination: "<<dest;
    di=opendir(source.c_str());
    struct stat fileInfo;
    if(di){
        while((dir=readdir(di))){
            string fname = string(dir->d_name);
            string finalF = source + "/" + fname;
            lstat(finalF.c_str(),&fileInfo);
            if(S_ISDIR(fileInfo.st_mode)){
                if( (fname == ".") || (fname == "..") ){
                    continue;
                }
                mkdir((dest+ "/" + fname).c_str(),0755);
                copy_dir_files(source+ "/" +fname , dest+ "/" +fname);
            }
            else{
                copy_File(source+"/"+fname,dest + "/" + fname);
            }
        }
    }
    else{
        printError("Cannot Open Directory");
        
        return;
    }
    closedir(di);
    return;
}

void copy_directory(string source,string dest){
   int len = commandsIn.size();
   int last;
    for(int i=0; i<source.size();i++){
        if(source[i]=='/'){
            last=i;
        }   
    }
   
    string  finalC = dest + "/" + source.substr(last+1);

    mkdir(finalC.c_str(),0755);
    copy_dir_files(source,finalC);
}

void copyFunc(){
    int len = commandsIn.size();
    int last,flag=0;
    string dest = commandsIn[len-1];
    string absDest=getAbsolutePath(dest);
    string copyloc;
    struct stat fileStat;
    int i=1;
    while(i<len-1){
        string source = commandsIn[i];
        copyloc = getAbsolutePath(commandsIn[i]);
        // cout<<copyloc<<""<<absDest;

        lstat(copyloc.c_str(),&fileStat);
        if(S_ISDIR(fileStat.st_mode))
            copy_directory(copyloc,absDest);
        else{
            for(int i=0; i<source.size();i++){
                if(source[i]=='/'){
                    last=i;
                    flag=1;
                }   
            }
            if(flag){
                source = source.substr(last+1);
                copy_File(copyloc,absDest+"/"+source);
            }
            else{
                copy_File(copyloc,absDest+"/"+commandsIn[i]);
            }
        }

        
        i+=1;
    }
    return;
}

/******************************MOVE PROCESSING ************************************************/
void move_file(string source,string dest){
    copy_File(source,dest);
    unlink(source.c_str());
}


void move_dir(string source, string dest){
    copy_directory(source,dest);
    delete_directory(source);
    rmdir(source.c_str());
    return;
}

void moveFunc(){
    int len = commandsIn.size();
    string des = commandsIn[len - 1];
    string absdes = getAbsolutePath(des);
    struct stat fileStat;
    int i = 1,last,flag=0;
    while(i<len-1){
        string src = commandsIn[i];
        string absSrc = getAbsolutePath(src);
        lstat(absSrc.c_str(),&fileStat);
        if(S_ISDIR(fileStat.st_mode)){
            move_dir(absSrc,absdes);
        }
        else{
            for(int i=0; i<src.size();i++){
                if(src[i]=='/'){
                    last=i;
                    flag=1;
                }   
            }
            if(flag){
                src = src.substr(last+1);
                move_file(absSrc,absdes+"/"+src);
            }
            else{
                move_file(absSrc,absdes+"/"+commandsIn[i]);
            }

        }
        i+=1;
    }
    return;
}



/******************************** PROCESSING DIFFERENT COMMANDS ****************************/
void processCommand(string comType){
    if(comType=="copy"){
        if(commandsIn.size()<3){
            printError("Insufficient Parameters!!");
            return;
        }
        
        copyFunc();
        if(error==0){
            moveCursor(0,E.commandCursorY-5);
            string sucmgs="File Copied Successfully!!";
             write(STDOUT_FILENO, &sucmgs[0] ,sucmgs.size());
        }
        moveCursor(0,E.commandCursorY);
        return;
    }

    else if(comType=="move"){
        if(commandsIn.size()<3){
            printError("Insufficient Parameters!!");
            return;
        }
        moveFunc();
        if(error==0){
            moveCursor(0,E.commandCursorY-5);
            string sucmgs="File Moved Successfully!!";
             write(STDOUT_FILENO, &sucmgs[0] ,sucmgs.size());
        }
         moveCursor(0,E.commandCursorY);
        return;
    }
    else if(comType=="delete_file"){
        if(commandsIn.size()<2){
            printError("Insufficient Parameters!!");
            return;
        }
        bool status;
        string des = getAbsolutePath(commandsIn[1]);
        status = delete_file(des);
        if(status){
            moveCursor(0,E.commandCursorY-5);
            string sucmgs="File Deleted Successfully!!";
            write(STDOUT_FILENO, &sucmgs[0] ,sucmgs.size());
            moveCursor(0,E.commandCursorY);
        }
         
        else{
            printError("Unable to delete File!");
        }
    }

    else if(comType == "delete_dir"){
        if(commandsIn.size()<3){
            printError("Insufficient Parameters!!");
            return;
        }
        string des = getAbsolutePath(commandsIn[1]);
        if(des==cwd){
            printError("Cannot Delete current working directory");
            return;
        }
        else{
            delete_directory(des);
            if(error==0){
            rmdir(des.c_str());
            moveCursor(0,E.commandCursorY-5);
            string sucmgs="Directory Deleted Successfully!!";
            write(STDOUT_FILENO, &sucmgs[0] ,sucmgs.size());
            moveCursor(0,E.commandCursorY);
            }
        }
        return;
    }
    else if(comType == "create_file"){
        if(commandsIn.size()<3){
            printError("Insufficient Parameters!!");
            return;
        }
        create_file();
        if(error==0){
            moveCursor(0,E.commandCursorY-5);
            string sucmgs="File Created Successfully!!";
            write(STDOUT_FILENO, &sucmgs[0] ,sucmgs.size());
            moveCursor(0,E.commandCursorY);
        }
        return;
    }
    else if(comType == "create_dir"){
        if(commandsIn.size()<3){
            printError("Insufficient Parameters!!");
            return;
        }
        create_dir();
        if(error==0){
            moveCursor(0,E.commandCursorY-5);
            string sucmgs="File Created Successfully!!";
            write(STDOUT_FILENO, &sucmgs[0] ,sucmgs.size());
            moveCursor(0,E.commandCursorY);
        }

    }

    else if(comType=="goto"){
        if(commandsIn.size()<2){
            printError("Insufficient Parameters!!");
            return;
        }
        string des = getAbsolutePath(commandsIn[1]);
        gotoFunc(des);
    }
    else if(comType=="rename"){
        if(commandsIn.size()<3){
            printError("Insufficient Parameters!!");
            return;
        }       
        string torename = getAbsolutePath(commandsIn[1]);
        string withname = getAbsolutePath(commandsIn[2]);
        if(rename(torename.c_str(),withname.c_str())==-1){
            printError("Rename Operation Failed!!");
        }
        else{
            moveCursor(0,E.commandCursorY-5);
            string sucmgs="Rename Operation Successful!!";
            write(STDOUT_FILENO, &sucmgs[0] ,sucmgs.size());
            moveCursor(0,E.commandCursorY);
        }
        return;
    }
    else if(comType=="search"){
        if(commandsIn.size()<2){
            printError("Insufficient Parameters!!");
            return;
        }
        string toSearch = getAbsolutePath(commandsIn[1]);
        string startPoint = string(cwd);
        bool find = search(startPoint,toSearch);
        if(find){
             moveCursor(0,E.commandCursorY-5);
            string sucmgs="True ";
            write(STDOUT_FILENO, &sucmgs[0] ,sucmgs.size());
            moveCursor(0,E.commandCursorY);
        }
        else{
            moveCursor(0,E.commandCursorY-5);
            string sucmgs="False";
            write(STDOUT_FILENO, &sucmgs[0] ,sucmgs.size());
            moveCursor(0,E.commandCursorY);
        }
    }

    else{
        printError("Invalid Input");
        return;
    }
}


/************************COMMAND MODE INPUT PROCESS***************************************/
void splitCommand(string command){
    istringstream ss(command);
  
    string word;

    while (ss >> word) 
    {   
        
        commandsIn.push_back(word);
    }
}

string comProcessKey(){
    char c;
    string command="";
    while (true){
        c=cin.get();
        if(c==13){
            return command;
        }
        if(c==27){
            return "Normal";
            break;
        }
        if(c==127){
            if(E.commandCursor==0)
                continue;
            command.pop_back();
            int cursorpos = command.size();
            write(STDOUT_FILENO, "\x1b[2J", 4);
            moveCursor(E.hCenter,E.screen_rows);
            write(STDOUT_FILENO,"Command Mode",12);
            moveCursor(0,E.commandCursorY); 
            write(STDOUT_FILENO, &command[0], command.size());
           // E.commandCursor-=1;
            E.commandCursor=cursorpos;
            moveCursor(E.commandCursor+1,E.commandCursorY);
        }
        else{
            command+=c;
            cout<<c;
            E.commandCursor+=1;
            moveCursor(E.commandCursor,E.commandCursorY);
            
        }
    }
}

/************************************ COMMAND MODE ***************************************/
void commandMode(){
            write(STDOUT_FILENO, "\x1b[2J", 4);
    while(1){
        error=0;
        moveCursor(E.hCenter,E.screen_rows);
        write(STDOUT_FILENO,"Command Mode",12);
        E.commandCursor=0;
        commandsIn.clear();
        moveCursor(E.commandCursor,E.commandCursorY);
    
        string com=comProcessKey();
        string comType;
        if(com==""){
            printError("Invalid input");
            continue;
        }
        if(com=="Normal"){
            refreshScreen();
            // enableNormalMode();
            getcwd(cwd, cwdSize);  
            processCurrDirectory(cwd);
            break;
        }
        else{
            cout<<"\33[2K";
        }
        splitCommand(com);
        comType = commandsIn[0];
        processCommand(comType);
    }
}


/******************************** ERROR HANDLING **********************/
void printError(string s)
{
    moveCursor(0,E.commandCursorY-5);
    s = "Error: " + s;
    write(STDOUT_FILENO,&s[0],s.size());
    error=1;
    return;
}


/********************* INPUT PROCESSING *******************************/
//read character
int readKey()
{
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1) != 1))
    {
        if (nread == -1 && errno != EAGAIN)
            printError("Unable to read");
    }
    if (c == '\x1b')
    {
        char seq[2];
        if (read(STDIN_FILENO, &seq[0], 1) != 1)
            return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1)
            return '\x1b';

        if (seq[0] == '[')
        {
            switch (seq[1])
            {
            case 'A':
                return ARROW_UP;
            case 'B':
                return ARROW_DOWN;
            case 'C':
                return ARROW_RIGHT;
            case 'D':
                return ARROW_LEFT;
            }
        }
        return '\x1b';
    }
    else 
    {
        return c;
    }
}


/************************ process Keyboard input *****************************/
void processKeyPress()
{
    int c = readKey();
    switch (c)
    {
        case 'q':
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;

        case ARROW_UP:
        case ARROW_DOWN:

            posCursor(c);
            moveCursor(E.cx, E.cy);
            break;
        
        case ARROW_RIGHT:
            moveForward();
            break;
        case ARROW_LEFT:
            moveBackward();
            break;

        //Enter Key
        case 13:
            onEnter();
            break;

        //Backspace
        case 127:
            goBackOneLevel();
            break;

        case 'h':
            home();
            break;

        case 'k':
            scrollUp();
            break;
        
        case 'l':
            scrollDown();
            break;

        case ':':
            commandMode();
            break;
    }
    

    
}


void initialize()
{
    // write(STDOUT_FILENO, "\x1b[H", 3);
    E.cx = 0;
    E.cy = 0;
    E.top = 0;
  
    if (getWindowSize(&E.screen_rows, &E.screen_cols) == -1)
        printError("Unable to get Window Size");
    E.bottomMax = 30;
    E.bottom=E.top+E.bottomMax;
    E.hCenter = E.screen_cols / 2;
    appStartPath = string(cwd);
    E.commandCursor=0;
    E.commandCursorY=E.screen_rows-2;
    // homePath=string(getHomeDir());
   // processCurrDirectory(cwd);
}


/********************* MAIN ********************************/
int main()
{
    refreshScreen();
    enableNormalMode();
    getcwd(cwd, cwdSize);
    initialize();
    processCurrDirectory(cwd);

    while (1)
    {
        processKeyPress();
    }    
    return 0;
}