/***************************************************************************/
/* This menu program has been created to allow the user to create a text   */
/* file that is read in and displayed as a menu.                           */
/* It uses curses so it should be portable across Unix's and terminals.    */
/*-------------------------------------------------------------------------*/
/* Author  : Steve Harris (steve.harris@shaw.ca)                           */
/* Version : 1.7                                                           */
/* Date    : 20th July 1996                                                */
/***************************************************************************/
/* Version 1.1 - Steve Harris (13/11/96)                                   */
/* 1. Changed getch to scanw and added atoi to cope with string and number */
/* input.                                                                  */
/* 2. Tried using switch/case for menu selection but failed.               */
/* (Suspect the problem is due the variety of option for exiting the menu) */
/* 3. Added cp - change password and OFF/off/Q/q options.                  */
/***************************************************************************/
/* Version 1.2 - Steve Harris (14/11/96)                                   */
/* 1. Added function to draw_bottom_line to clear the menu option after the*/
/*    refresh - uses clrtoeol().                                           */
/* 2. Changed the quit option to use die() instead of exit(0).             */
/***************************************************************************/
/* Version 1.3 - Steve Harris (14/11/96)                                   */
/* 1. Convert the menu body drawing to a function.                         */
/* 2. Call it from thew construct menu function.                           */
/* 3. Change code to draw menu in different places and styles (spacing)    */
/*    depending on the number of menu options.                             */
/***************************************************************************/
/* Version 1.4 - Steve Harris (14/11/96)                                   */
/* 1. Changed the menu style to do side by side columns to balance the menu*/
/*    when using more than 8 menu options.                                 */
/***************************************************************************/
/* Version 1.5 - Steve Harris (14/11/96)                                   */
/* 1. Added comments.                                                      */
/* 2. Moved heading termination to file read function because it carried on*/
/*    truncating itself when <cr> was pressed on its own.                  */
/***************************************************************************/
/* Version 1.6 - Steve Harris (14/11/96)                                   */
/* 1. Added the prompt to change the password.                             */
/* 2. Hard coded the kerberos password option.                             */
/***************************************************************************/
/* Version 1.7 - Steve Harris (15/11/96)                                   */
/* 1. Changed the if/else tests for menu choices to be tidier.             */
/***************************************************************************/
/* Version 1.8 - Steve Harris (28/2/97)                                    */
/* 1. Introduced isendwin,wrefresh,doupdate because of intermittent screen */
/*    redraw problems.                                                     */
/***************************************************************************/
/* Version 1.9 - Steve Harris (02/6/2020)                                  */
/* 1. Added a few includes to get this up and running on Ubuntu 16.4       */
/***************************************************************************/

#include <curses.h>
#include <signal.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/************************************/
/*Setup structures for the menu file*/
/*and other parameters.             */
/************************************/
#define MAX_NO_ITEMS 30

struct menu_item
{
	char description[30];
	char command[80];
};

struct menu_item menu[MAX_NO_ITEMS];

/*Setup variable to use in code*/
int center_x;
int die(), count, total_no_of_items; 
int *ch;		/*Used for EOF*/
WINDOW *subone;		/*Window pointer for menu*/
char from_term[3]; 	/*Characters read in from terminal*/
int choice;		/*Used to hold number of menu choice*/
int item_step;		/*Gap plus one between menu options*/

/*Used in constructing menu - messy isn't it*/
char line[81] = "                                                                                ";
char prompt_line[] = "Enter Option : ";
char exit_line[]="(OFF to quit)";
char password_prompt[]="CP to change password";

/*Menu file related settings*/
FILE *menu_file; 
char menu_heading[80];
char menu_description[80];
char *file_name;

/*Variables to hold environment variables*/
char *login_name, *login_term;

/******************************************/
/*This is main() - where the real work is */
/*done.                                   */
/******************************************/
main(int argc, char *argv[])
{
	/*Record menu file name*/
	file_name=argv[1];

	/*Setup curses and interrupts*/
	initscr();
	signal(SIGINT, die);

	/*Setup the variables depending on the  */
	/*environment settings.                 */
	login_name=getenv("LOGNAME");
	login_term=getenv("TTY"); 
	
	/*Read in the menu file*/
	read_menu_file();

	/*Draw menu*/
	while(1)
	{
		/*Setup menu outline*/
		construct_menu(file_name);

		if (total_no_of_items <= 8)
		{
			/*Draw single column, double spaced*/
			draw_menu_body(5,30,2,1,8);
		}
		else
		{
			/*Draw two column,single spaced*/
			draw_dual_body(5,7);
		}

		/*Position cursor in prompt and draw*/
		move(23,15);
		refresh();

		/*Get the menu option*/
		get_option();

		/*Clear the from_term and choice*/
		choice=0;
		from_term[0]='\0';

	} 
}

/********************/
/*Read the menu file*/
/********************/
read_menu_file()
{
	/* Open file and read in data - send error if not there.*/	
	menu_file = fopen(file_name,"r");

	if (menu_file == NULL)
	{
		printf("Cannot open your menu file.\n");
		printf("Usage : menu 'menu text_file'\n\n");
		endwin();
		exit(8);
	};

	/*Read in first two heading lines from file and then read*/
	/*the rest into our array of structures*/
	fgets(menu_heading, sizeof(menu_heading), menu_file);
	fgets(menu_description, sizeof(menu_description), menu_file);

	count=0;

	while (count < MAX_NO_ITEMS)
	{
		ch = fgets(menu[count].description, sizeof(menu[count].description), menu_file);
		(void)fgets(menu[count].command, sizeof(menu[count].command), menu_file);			
		menu[count].command[strlen(menu[count].command)-1] = '\0';

		/*Check for end of file*/
		if (ch  == NULL) break;

		count++;
	};

	/*Truncate the menu headings*/
	menu_heading[strlen(menu_heading)-1] = '\0';
	menu_description[strlen(menu_description)-1] = '\0';

	fclose(menu_file);
	total_no_of_items=count;
}

/****************************************/
/*Following function contructs the menu.*/
/****************************************/
construct_menu(char menu_file_name[])
{
	/*Creates the main menu window*/

	/*Create the top line*/
	move(0,0);
	attron(A_UNDERLINE);
	addstr(line);
	move(0,0);
	addstr(login_name);
	move(0,80-strlen(login_term));
	addstr(login_term);

	find_center(menu_heading);
	move(0,center_x);
	addstr(menu_heading);

	find_center(menu_description);
	move(2,center_x);
	addstr(menu_description);
	attroff(A_UNDERLINE);

	/*Put in prompt line.*/
	move(21,0);
	attron(A_UNDERLINE);
	addstr(line);
	find_center(password_prompt);
	move(21,center_x);
	addstr(password_prompt);
	attroff(A_UNDERLINE);

	/*Draw the bottom line*/
	draw_bottom_line();

}

/***********************************/
/*Function to draw a dual menu body*/
/***********************************/
draw_dual_body(int start_y_dual,int start_x_dual)
{
	int right_hand, left_hand, temp_count, right_y;

	/*Work out left and right hand sides*/
	right_hand=total_no_of_items/2;
	left_hand=total_no_of_items-right_hand;

	/*Put in menu options*/
	right_y=start_y_dual;

	move(start_y_dual,start_x_dual);

	for(temp_count=1;temp_count<=left_hand;temp_count++)
	{
		/*Left hand side*/
		printw("%d.",temp_count);
		move(start_y_dual,start_x_dual+4);
		addstr(menu[temp_count-1].description);
		
		start_y_dual++;

		move(start_y_dual,start_x_dual);
	}


	move(right_y,start_x_dual+44);

	for(temp_count=left_hand+1;temp_count<=total_no_of_items;temp_count++)
	{
		/*Right hand side*/
		move(right_y,start_x_dual+44);
		printw("%d. ", temp_count);
		move(right_y,start_x_dual+48);
		addstr(menu[temp_count-1].description);
		
		right_y++;

		move(right_y, start_x_dual+48);	
	}
}


/***************************************************/
/*Function to draw the menu body in a single colmun*/
/***************************************************/
draw_menu_body(int start_y, int start_x, int item_gap, int start_option, int end_option)
{
	/*Draw rest of menu*/
	/*Put in menu options*/
	
	move(start_y,start_x);

	while ((start_option <= end_option) && (start_option <= total_no_of_items))
	{
		printw("%d.",start_option);
		move(start_y,start_x+4);
		addstr(menu[start_option-1].description);
		start_y+=item_gap;
		move(start_y,start_x);
		start_option++;
	}

}

/************************************************************************/
/*Draw the bottom line - in a function because its used in more than one*/
/*place. It dosen't do a refresh the calling function should do it.     */
/************************************************************************/
draw_bottom_line()
{
	move(23,0);
	clrtoeol();
	move(23,0);
	addstr(prompt_line);
	move(23,80-strlen(exit_line));
	addstr(exit_line);
}

/************************************************************************/
/*function to get response from the user and load the appropriate option*/
/************************************************************************/
get_option()
{
	/*Setup and get response from user*/
	raw();
	echo();
	nl();
	scanw("%3s",from_term);
	printw("%3s",from_term);
	choice=atoi(from_term);
	
	/*Process menu choice*/

	if ((strcmp(from_term,"q") == 0) ||
	    (strcmp(from_term,"Q") == 0) ||
	    (strcmp(from_term,"off") == 0) ||
	    (strcmp(from_term, "OFF") == 0))
	{
		die();
	}
	else
	{
		if ((strcmp(from_term,"cp") ==0) ||
	    	   (strcmp(from_term,"CP") ==0))
		{
			endwin();
			system("clear;kerbpass $LOGNAME");
			doupdate();
		}
		else
		{
			if (choice != 0)
			{
				endwin();
				system(menu[choice-1].command);
				doupdate();
			}
		}
	}
	draw_bottom_line();
}

/*********************************/
/*Function to service interrupt  */
/*or to simply terminate.        */
/*********************************/
die()
{
	endwin();
	exit(0);
}

/***************************************/
/*Find the center of whats passed to it*/
/***************************************/
find_center(char center_of_what[])
{
	center_x = (80-(strlen(center_of_what)-1))/2;
}
		
