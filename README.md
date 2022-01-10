## Linux Based File Explorer

### How to execute
* Navigate to folder where main.cpp is present.
* Compile Using following command : `g++ main.cpp` .
* Run `./a.out` file.
## Note:  Only full mode is supported, minimizing or changing screen size might not work properly.

### ABOUT
A functional File Explorer Application, with a restricted feature set. It has two modes:
* Normal Mode (By default)
* Command Mode
The last line of the display screen is used as status bar  to show which mode we are currently in.

### NORMAL MODE
* Display list of directories and files present in current folder with following information respectively:
Permissions,Directory/File Size, Ownership User and Group, File/Directory Modified Time, Directory/ File Name.
* Directory name is shown in blue color.
* Different Key presses functionality:
    * `k` : In case of vertical overflow pressing k we can scroll up.
    * `l` : In case of vertical overflow pressing l we can scroll down.
    * `UP Arrow key`: used for moving cursor up to desired file or directory
    * `DOWN Arrow key`: used for moving cursor down to desired file or directory
    * `ENTER KEY`: When user presses it, and if its the file it will open in default editor and if directory it will open that direcotry and show its content.
    * `LEFT ARROW KEY`: Goes back to the previously visited directory
    * `RIGHT ARROW KEY`: Goes to next directory
    * `BACKSPACE KEY`: Takes user up to one level
    * `h`: (Home) should Take the user to the home folder (the folder where the application was started).
## Note: 
* At maximum 30 files/ directory can be shown and if current directory has more files than it can be navigated using `k` and `l` keys.
* root of application is `/`.
* home of application is path where the application was started.
* Normal Mode Also shows `current directory` you are in.

### Command Mode
* The mode can entered from Normal mode on pressing `:`.
## Following commands are supported:
* COPY: `copy <file_name(s)> <target_directory_path>`
* MOVE: `move <file_name(s)> <target_directory_path>`
* RENAME: `rename <old_file_name> <new_file_name>`
* CREATE FILE: `create_file <file_name> <destination_path>`
* CREATE DIRECTORY: `create_dir <diectory_name> <destination_path>`
* DELETE FILE: `delete_file <file_path>`
* DELETE DIRECTORY: `delete_dir <directory_path>`
* SEARCH: `search <file_name / directory_name>`
* GOTO: `goto <directory_path>`
* Pressing ESC KEY takes user back to Normal Mode.
## Note:
* ~/path : refers to path from home directory(home is the folder where the application was started).
* /path --> refers to path from root, which is taken as `/`.
* Output of search will print `True` if file/Directory is present or `False` if not present