#define MAX_PATH_LENGTH 256

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
} directory_t;

static void init_tr();
static void destroy_tr();
static void init_directory(directory_t * dir);
static void destroy_directory(directory_t * dir);
static int load_directory(directory_t * dirt, const char * dir);
static bool isValidDir(char * dir);
static char * _strdup(const char * str);

/*
 * This function initializes all the default settings
 */
static void 
init_tr()
{
	initscr();
	cbreak();
	noecho();
	keypad(stdscr,TRUE);	 // special keys (eg. KEY_DOWN, KEY_UP, etc)
	scrollok(stdscr,TRUE); 	 // allow the window to scroll
	nodelay(stdscr,TRUE);    // non-blocking input

	// set up colors
	if (has_colors()) {
		//start_color();
		//init_pair(0, COLOR_BLACK, COLOR_WHITE);
	}
}

/*
 * This function destroys and frees all resources held by the program
 */
static void 
destroy_tr()
{
	// restore previous terminal settings
	endwin();
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
 * Returns 1 on success, 0 on failure
 */
static int
load_directory(directory_t * dir_t, const char * dir)
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
		while ((ep = readdir(dp))) {
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
					if (!load_directory(&dir_t->children[numc-1],childpath))
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
 * Helper function that checks if a path is a directory
 */
static bool 
isValidDir(char * dir)
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

	int ch;
	directory_t * dt;
	while (true) {
		if ((ch = getch() == ERR)) {
			// user hasn't pressed a key
		}
		else {
			// user pressed a key
			attroff(A_STANDOUT);
			wmove(stdscr,0,0);

			dt = malloc(sizeof(directory_t));
			init_directory(dt);
			load_directory(dt,"/home/dan/dirtest");
			for (int i = 0; i < dt->num_children; ++i) {
				wmove(stdscr,i,0);
				wprintw(stdscr,"%s\n",dt->children[i].dirname);
			}

			wrefresh(stdscr);
			sleep(4);
			break;
		}
	}

	// release resources
	destroy_directory(dt);
	free(dt);
	destroy_tr();	
}
