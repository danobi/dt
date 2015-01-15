#define MAX_PATH_LENGTH 256
#define LINE_PADDING_WIDTH 3
#define LOAD_DIR_DEPTH 3

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
	bool isChildrenLoaded;
} directory_t;

static void tr_init(void);
static void tr_destroy(void);
static void directory_init(directory_t * dir);
static void directory_destroy(directory_t * dir);
static int  directory_load(directory_t * dirt, const char * dir, int depth);
static void directory_display(const directory_t * dir);
static void directory_display_helper(const directory_t * dir, int depth);
static void directory_moveup(directory_t * rootdir, directory_t ** curdir);
static void directory_movedown(directory_t * rootdir, directory_t ** curdir);
static void directory_action(directory_t * curdir);
static void directory_enter(directory_t * curdir);
static void screen_clear(void);
static void input_handle(void);
static char * generate_padding(int depth);
static bool isValidDir(const char * dir);
static char * _strdup(const char * str);

//const char * starting_directory = "/home/dan";
const char * starting_directory = "/home/dan/dirtest";
directory_t * root_dir;
directory_t * cur_sel_dir;

/*
 * This function initializes all the default settings
 */
static void 
tr_init(void)
{
	initscr(); 				 // begin ncurses session
	noecho(); 			     // prevent echoing of pressed keys on screen
	keypad(stdscr,TRUE);	 // special keys (eg. KEY_DOWN, KEY_UP, etc)
	scrollok(stdscr,TRUE); 	 // allow the window to scroll
	nodelay(stdscr,TRUE);    // non-blocking input

	// set up colors
	if (has_colors()) {
		//start_color();
		//init_pair(0, COLOR_BLACK, COLOR_WHITE);
	}

	// alloc and init root data structure
	root_dir = malloc(sizeof(directory_t));
	directory_init(root_dir);
	directory_load(root_dir,starting_directory,LOAD_DIR_DEPTH);
	
	// initially the selected directory is root
	cur_sel_dir = root_dir;
	cur_sel_dir->isSelected = true;
	cur_sel_dir->isExpanded = true;
}

/*
 * This function destroys and frees all resources held by the program
 */
static void 
tr_destroy(void)
{
	// restore previous terminal settings
	endwin();

	// destroy root_dir
	directory_destroy(root_dir);
	free(root_dir);
}

/*
 * This function inits a directory_t struct
 */
static void
directory_init(directory_t * dir) 
{
	dir->dirname = NULL;
	dir->children = NULL;
	dir->fullpath = NULL;
	dir->num_children = 0;
	dir->isExpanded = false;
	dir->isSelected = false;
	dir->isChildrenLoaded = false;
}

/*
 * This function destroys the resources held by parameter dir
 * This will NOT free the memory occupied by parameter dir
 */
static void
directory_destroy(directory_t * dir)
{
	free(dir->dirname);
	free(dir->fullpath);
	for (int i = 0; i < dir->num_children; ++i) {
		directory_destroy(&dir->children[i]);
	}
	free(dir->children);
}

/*
 * This function loads the data structure behind this program
 * directory_init(..) MUST be called on the struct before this function is!
 * Parameter depth specifies how many directories deep the data structure will be loaded, parameter
 * 	   dir is at depth level 0. So with depth level 2, this function would load dir, dir's children,
 *     and dir's children's children. If the children are not loaded due to depth level, what that 
 *     really means is that the children's children are not loaded, but the children's fullpath and
 *     dirname are stored correctly
 * Behavior is undefined if you call directory_load(..) on a directory_t struct that has already
 *     been loaded
 * Returns 1 on success, 0 on failure
 */
static int
directory_load(directory_t * dir_t, const char * dir, int depth)
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
		while ((ep = readdir(dp)) && depth > 0) {
			// recursively store children if not . or ..
			if (strcmp(ep->d_name,".") && strcmp(ep->d_name,"..") && *(ep->d_name) != '.') {  
				// create new path
				char childpath[strlen(dir_t->fullpath)+strlen(ep->d_name)+2];
				sprintf(childpath,"%s/%s",dir_t->fullpath,ep->d_name);

				// only load struct if child path is a directory
				if (isValidDir(childpath)) {
					int numc = ++dir_t->num_children;
					dir_t->children = realloc(dir_t->children,numc * sizeof(directory_t));
					directory_init(&dir_t->children[numc-1]);
					if (!directory_load(&dir_t->children[numc-1],childpath,depth-1))
						returnval = 0;
					dir_t->isChildrenLoaded = true;
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
static void directory_display(const directory_t * dir)
{
	screen_clear();
	wmove(stdscr,0,0);
	directory_display_helper(dir,0);
}

/*
 * Recursive helper function for directory_display
 * ypos is the row we're in on the screen
 * depth is how deep in the folder structure we are, top level directory (ie where tr was run) is 
 *     depth 0
 */
static void directory_display_helper(const directory_t * dir, int depth)
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
	// turn on temporary formatting
	if (dir->isSelected) {
		attron(A_STANDOUT);
		attron(A_UNDERLINE);
	}
	int y,x=3;y=x;  	// to silence warnings
	getyx(stdscr,y,x);
	wmove(stdscr,y,0);
	wprintw(stdscr,"%s\n",line);
	free(line);
	// turn off temporary formatting
	if (dir->isSelected) {
		attroff(A_STANDOUT);
		attroff(A_UNDERLINE);
	}

	// print children directories if necessary
	if (dir->isExpanded) {
		for (int i = 0; i < dir->num_children; ++i) {
			directory_display_helper(&dir->children[i],depth+1);
		}
	}
}

/*
 * This function will change curdir to be the directory "above" the current curdir in the 
 *     (user set) expanded view
 * The current strategy is to begin at the root dir and BFS the expanded view, keeping track of
 *     the current dir we're on as well as the previous one. Once we find the curdir, the previous
 *     dir must be the new curdir
 * rootdir should be the global variable root_dir
 * curdir should be the location of the pointer of the global variable cur_sel_dir, since this function
 *     plans on changing the value of cur_sel_dir
 */
static void 
directory_moveup(directory_t * rootdir, directory_t ** curdir)
{
	// TODO
}

/*
 * This function will change curdir to be the directory "below" the current curdir in the 
 *     (user set) expanded view
 * The current strategy is to begin at curdir and see if curdir is expanded, if it is, getting the 
 *     directory "below" is trivial. If curdir is not expanded, we must BFS the expanded view keeping 
 *     track of the previous dir and the current dir we're on until we find the curdir. Then 
 *     the previous dir must be the new curdir
 * rootdir should be the global variable root_dir
 * curdir should be the location of the pointer of the global variable cur_sel_dir, since this function
 *     plans on changing the value of cur_sel_dir
 */
static void 
directory_movedown(directory_t * rootdir, directory_t ** curdir)
{
	// TODO
}

/*
 * This function will either expand the currently selected directory in the expanded view,
 *     or it will hide the currently selected directory in the expanded view, depending on the state
 *     ie. if its expanded, this function will hide it, and vis versa
 * Furthermore, this function will load the next relevant directory_t structures if necessary
 * curdir is the currently selected directory
 */
static void 
directory_action(directory_t * curdir)
{
	// flip the flag 
	curdir->isExpanded = !curdir->isExpanded;

	// load next level of directories if necessary
	if (!curdir->isChildrenLoaded) {
		for (int i = 0; i < curdir->num_children; ++i) {
			directory_load(&curdir->children[i],curdir->children[i].fullpath,LOAD_DIR_DEPTH);
		}
	}
}

/* 
 * This function will change the current working directory to the currently selected directory
 * If we're already in this directory, then this function will do jack shit
 * curdir is the currently selected directory
 */
static void 
directory_enter(directory_t * curdir)
{
	// TODO
}

/*
 * This function is a breakable infinite loop that handles all keyboard input, essentially the control 
 *     point of tr
 */
static void input_handle(void)
{
	int ch;
	bool running = true;
	while (running) {
		ch = getch();
		switch (ch) {
			case ERR:   // no user input
				break;
			case 'j':  	// move down an entry
				directory_movedown(root_dir,&cur_sel_dir);
				break;
			case 'k': 	// move up an entry
				directory_moveup(root_dir,&cur_sel_dir);
				break;
			case ' ': 	// space key
				directory_action(cur_sel_dir);
				break;
			case '\n':  // enter key
				directory_enter(cur_sel_dir);
				break;
			case 'q':   // quit key
				running = false;
				break;
			default:    // unrecognized input
				break;
		}
		
		// update screen
		directory_display(root_dir);
		wrefresh(stdscr);
	}
}

/*
 * Clears the entire screen, line by line
 */
static void screen_clear(void)
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
	tr_init();
	input_handle(); 	// this is the main event loop
	tr_destroy();	
}
