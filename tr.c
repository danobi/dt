#define MAX_PATH_LENGTH 256
#define LINE_PADDING_WIDTH 3

#include <curses.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>

typedef struct _directory_t {
	char * dirname; 					// name of current directory
	char * fullpath; 					// full path of current directory
	struct _directory_t * children;   	// array of children structs
	int num_children; 					// number of children directories
	bool isExpanded; 					// bool to represent if directory is expanded 
	bool isSelected; 					// bool to represent if this directory is selected
} directory_t;

static void init_tr(void);
static void destroy_tr(void);
static void init_directory(directory_t * dir);
static void destroy_directory(directory_t * dir);
static int load_directory(directory_t * dirt, const char * dir, int depth);
static void display_directory(const directory_t * dir);
static void display_directory_helper(const directory_t * dir, int ypos, int depth);
static void clear_screen(void);
static void handle_input(void);
static char * generate_padding(int depth);
static bool isValidDir(const char * dir);
static char * _strdup(const char * str);

const char * starting_directory = "/dan/home";
directory_t * gbl_dir;

/*
 * This function initializes all the default settings
 */
static void 
init_tr(void)
{
	initscr();
	cbreak();
	noecho(); 			     // prevent echoing of pressed keys on screen
	keypad(stdscr,TRUE);	 // special keys (eg. KEY_DOWN, KEY_UP, etc)
	scrollok(stdscr,TRUE); 	 // allow the window to scroll
	nodelay(stdscr,TRUE);    // non-blocking input

	// set up colors
	if (has_colors()) {
		//start_color();
		//init_pair(0, COLOR_BLACK, COLOR_WHITE);
	}

	gbl_dir = malloc(sizeof(directory_t));
	init_directory(gbl_dir);
	load_directory(gbl_dir,starting_directory,1);
	display_directory(gbl_dir);
	wrefresh(stdscr);
}

/*
 * This function destroys and frees all resources held by the program
 */
static void 
destroy_tr(void)
{
	// restore previous terminal settings
	endwin();

	// destroy gbl_dir
	destroy_directory(gbl_dir);
	free(gbl_dir);
}

/*
 * This function inits a directory_t struct
 */
static void
init_directory(directory_t * dir) 
{
	dir->dirname = NULL;
	dir->children = NULL;
	dir->fullpath = NULL;
	dir->num_children = 0;
	dir->isExpanded = false;
}

/*
 * This function destroys the resources held by parameter dir
 * This will NOT free the memory occupied by parameter dir
 */
static void
destroy_directory(directory_t * dir)
{
	free(dir->dirname);
	free(dir->fullpath);
	for (int i = 0; i < dir->num_children; ++i) {
		destroy_directory(&dir->children[i]);
	}
	free(dir->children);
}

/*
 * This function loads the data structure behind this program
 * init_directory(..) MUST be called on the struct before this function is!
 * Parameter depth specifies how many directories deep the data structure will be loaded, parameter
 * 	   dir is at depth level 0. So with depth level 2, this function would load dir, dir's children,
 *     and dir's children's children
 * Behavior is undefined if you call load_directory(..) on a directory_t struct that has already
 *     been loaded
 * Returns 1 on success, 0 on failure
 */
static int
load_directory(directory_t * dir_t, const char * dir, int depth)
{
	int returnval = 1;

	// store current directory info
	dir_t->fullpath = _strdup((char*)dir);
	char * dn = (char*)dir;
	while (*dn) dn++; while (*dn != '/') dn--; dn++;
	dir_t->dirname = _strdup((char*)dn);

	// store children directory info
	struct dirent * ep;
	DIR * dp = opendir(dir);
	if (dp != NULL) {
		while ((ep = readdir(dp)) && depth >= 0) {
			// recursively store children if not . or ..
			if (strcmp(ep->d_name,".") && strcmp(ep->d_name,"..") && *(ep->d_name) != '.') {  
				// create new path
				char childpath[strlen(dir_t->fullpath)+strlen(ep->d_name)+2];
				sprintf(childpath,"%s/%s",dir_t->fullpath,ep->d_name);

				// only load struct if child path is a directory
				if (isValidDir(childpath)) {
					int numc = ++dir_t->num_children;
					dir_t->children = realloc(dir_t->children,numc * sizeof(directory_t));
					init_directory(&dir_t->children[numc-1]);
					if (!load_directory(&dir_t->children[numc-1],childpath,depth-1))
						returnval = 0;
				}
			}
		}
		closedir(dp);
	}
	else {
		perror("Couldn't open the directory");
		returnval = 0;
	}

	return returnval;
}

/*
 * Main function for drawing a directory onto stdscr
 * This function should not change the directory_t data structure at all
 * TODO: currently, the strategy is to redraw the entire screen whenever there is any keyboard
 *     action. Perhaps this can be improved later (ironic 'perhaps')
 */
static void display_directory(const directory_t * dir)
{
	clear_screen();
	wmove(stdscr,0,0);
	display_directory_helper(dir,0,0);
}

/*
 * Recursive helper function for display_directory
 * ypos is the row we're in on the screen
 * depth is how deep in the folder structure we are, top level directory (ie where tr was run) is 
 *     depth 0
 */
static void display_directory_helper(const directory_t * dir, int ypos, int depth)
{
	// generate the proper directory string
	char * line = generate_padding(depth);
	if (line != NULL) {
		line = realloc(line,(strlen(line) + strlen(dir->dirname) + 1) * sizeof(char));
		strcat(line,dir->dirname);
	}
	else 
		line = _strdup(dir->dirname);

	// format and print out directory string
	if (dir->isSelected) {
		attroff(A_NORMAL);
		attron(A_STANDOUT);
		attron(A_UNDERLINE);
	}
	else {
		attroff(A_STANDOUT);
		attroff(A_UNDERLINE);
		attron(A_NORMAL);
	}
	wmove(stdscr,ypos,0);
	wprintw(stdscr,"%s",line);
	free(line);

	// print children directories if necessary
	if (dir->isExpanded) {
		for (int i = 0; i < dir->num_children; ++i) {
			display_directory_helper(&dir->children[i],ypos+1,depth+1);
		}
	}
}

/*
 * This function is a breakable infinite loop that handles all keyboard input, essentially the control 
 *     point of tr
 */
static void handle_input(void)
{
	int ch;
	while (true) {
		if ((ch = getch() == ERR)) {
			// user hasn't pressed a key
		}
		else {
			// user pressed a key
			display_directory(gbl_dir);
			wrefresh(stdscr);
			sleep(4);
			break;
		}
	}
}

/*
 * Clears the entire screen, line by line
 */
static void clear_screen(void)
{
	for (int i = 0; i < LINES; ++i) {
		wmove(stdscr,i,0);
		for (int j = 0; j < COLS; ++j) {
			addch(' ');
		}
	}
}

/*
 * Returns a set number of spaces * depth as a cstring
 * Returns NULL if there is no padding
 */
static char * generate_padding(int depth)
{
	if (depth == 0) return NULL;
	char * re = malloc((LINE_PADDING_WIDTH * depth + 1) * sizeof(char));
	memset(re,' ',LINE_PADDING_WIDTH * depth * sizeof(char));
	re[LINE_PADDING_WIDTH * depth] = '\0';
	return re;
}

/*
 * Helper function that checks if a path is a directory
 */
static bool 
isValidDir(const char * dir)
{
	struct stat s;
	return stat(dir,&s) == 0 && S_ISDIR(s.st_mode);
}

/*
 * A simple reimplementation of strdup to silence compiler warnings
 */
static char * 
_strdup(const char * str)
{
	char * dup = malloc(strlen(str) + 1);
	if (dup)
		strcpy(dup, str);
	else 
		perror("Error -> strdup(..) can't malloc");
	return dup;
}

/*
 * Entry point
 */
int 
main(int argc, char * argv[])
{
	init_tr();
	handle_input(); 	// this is the main event loop
	destroy_tr();	
}
