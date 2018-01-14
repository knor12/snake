/*
 ============================================================================
 Name        : Snake Game
 Author      : Noreddine Kessa
 Version     :
 Copyright   : Do whatever you want with the code
 Description : the snake game in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <curses.h>
 
 
///////////////////Instructions///////////////////
 
 /*

this game uses ncurses library

1) install the library and headers, use this command
sudo apt-get install libncurses5-dev libncursesw5-dev


1)to compile this program using gcc, use.
gcc  -o snake  snake.c   -lncurses


*/


///////////////////game configuration///////////////////

#undef KEY_UP
#undef KEY_DOWN
#undef KEY_RIGHT
#undef KEY_LEFT


#define KEY_UP 65
#define KEY_DOWN 66
#define KEY_RIGHT 67
#define KEY_LEFT 68

//set this to false if you don't want to display game data
#define DISPLAY_GAME_DATE FALSE

#define SNAKE_INITIAL_SIZE 1
#define SNAKE_MAX_SIZE 800
#define SNAKE_LINK_CHARACTER "0"

//this is the interval of time between iterations at the start of the game
#define GAME_TIME_INTERVAL_START_SEC 0.5
//this is the interval of time between iterations right before the players wins the game
#define GAME_TIME_INTERVAL_END_SEC 0.05

//we don't want the trophy to be hidden in the wall
#define TROPHY_MIN_DISTANCE_FROM_WALL 5
#define TROPHY_MIN_VALUE 15
#define TROPHY_MAX_VALUE 30
//#define TROPHY_MIN_TIME_TO_LIVE 1
//#define TROPHY_MAX_TIME_TO_LIVE 9
#define TROPHY_MIN_TIME_TO_LIVE 20
#define TROPHY_MAX_TIME_TO_LIVE 30

///////////////////Model includes all structures used in the game///////////////////

enum Enum_Direction {
	Direction_up =0,
	Direction_down =1,
	Direction_left =2,
	Direction_right=3};

enum Enum_Game_Status{
	Game_Status_Running ,
	Game_Status_Stopped ,
	Game_Status_Lost,
	Game_Status_win
};


struct point {
	int x , y;
};



struct Sneak{
	//direction of where the snake is going
	volatile enum Enum_Direction Direction ;
	//location of the had of the snake at any given moment
	volatile int head_location_x;
	volatile int head_location_y;
	//size of the snke 
	volatile int size ;
	//this is an array that holds the cordnants of the links of the snake
	struct point links[SNAKE_MAX_SIZE];

};

struct Game{
	//status of the game, see Enum_Game_Status
	volatile enum Enum_Game_Status  status ;
	
	//seconds since the beginin of the game
	volatile int seconds ;
	
	//milliseconds since the last seconds
	//this is reset every second
	volatile int milli_seconds; 
	
	//the score the snake needs to be as long to win
	volatile int win_score;
	
	//the start interval between steps, in milli seconds 
	int start_timer_interval_mSec;
	
	//the interval between steps right before the snake wins
	int  end_timer_interval_mSec;
	
	//calculated during initializing, equates to by 
	//how much you decrease the interval between steps 
	//if the snake grew by one link
	volatile float increments_timer_intervals_mSec;
	
	//this the time between steps at any given moment
	volatile float current_time_interval_sec;
	
	//the width of the screen
	int window_max_x ;
	
	//the height of the screen
	int window_max_y;



};

struct Trophy{
	//indicate whether the throphy is on or of 
	// this toggles to give a blinking appearance 
	//of the trophy
	volatile int on_or_off;
	
	// the value of the trophy indicates by 
	//how mych the snake grows if it eats the trophy 
	volatile int value ;
	
	//how long with the trophy lives before 
	//it expires if it is eaten by the snake 
	volatile int time_to_live_secs;
	
	//location coordnants of the trophy 
	volatile struct point location;
};

///////////////////Global variables used in the program///////////////////
struct Trophy mytrophy;
struct Game mygame;
struct Sneak mySnake;
static struct itimerval delay;
WINDOW * mywindow ;

///////////////////Function prototypes///////////////////

/////////function that handle the controls of the game//////////

//initialization of data structures
void init_game();
void init_snake();

//generation of new items
int new_random_number(int lowest_val , int upper_val);
void new_trophy();

//game logic 
void game_logic();
void cleanup_exit();
void move_snake();
void feed_snake();
bool is_win_game();

//physics of the game and collisions
bool is_collision();
bool is_collison_snake_with_self();
bool is_collision_snake_with_outline();
bool is_collision_snake_with_trophy();

//timer stuff
void init_timer();
void timer_handler();


/////////function that handle the view of the game//////////
char draw_message_box(char * some_message);
void init_window();
void redraw_window();
void redraw_snake();
void redraw_outline();
void redraw_trophy();
void redraw_game_data();



///////// and this is main////////

int main(int argc , char * argv[]) {

	int c ;

	//Initialize the window;
	init_window();

	//initialize game status
	init_game();

	//create a new trophy
	new_trophy();

	//initialize the snake
	init_snake();


	//initialize the timer
	init_timer();

	//loop forever
	while(1){
		c = wgetch(mywindow);
		//wmove(mywindow , mySnake.head_location_x , mySnake.head_location_y);
		//wprintw(mywindow , "Character=%d", c);
		switch(c)
		{
		case KEY_UP:

			mySnake.Direction = Direction_up;

			break;
		case KEY_DOWN:
			mySnake.Direction = Direction_down;

			break;
		case KEY_LEFT:
			mySnake.Direction = Direction_left;

			break;
		case KEY_RIGHT:

			mySnake.Direction = Direction_right;

			break;

		case KEY_EXIT:
			cleanup_exit();
			break;
		default:
			//mvprintw(24, 0, "Charcter pressed is = %3d Hopefully it can be printed as '%c'", c, c);
			//refresh();
			break;
		}
	}



	puts("curses_library_testing"); /* prints curses_library_testing */
	return EXIT_SUCCESS;
}


void redraw_game_data(){

	int temp ;
	//Display points needed to win
	temp = mygame.win_score - mySnake.size;
	wmove(mywindow , 2, 2);
	wprintw(mywindow, "Points to win : %d" , temp );

	//Display snake speed in Hz
	temp = 1/mygame.current_time_interval_sec;
	wmove(mywindow , 4, 2);
	wprintw(mywindow, "Snake speed   : %d" , temp );

	//Trophy time to live
	temp = mytrophy.time_to_live_secs - mygame.seconds;
	wmove(mywindow , 6, 2);
	wprintw(mywindow, "Trophy to live: %d" , temp );

	//snake x
	temp = mySnake.head_location_x;
	wmove(mywindow , 8, 2);
	wprintw(mywindow, "Snake coord  x: %d" , temp );

	//trophy x
	temp = mytrophy.location.x;
	wmove(mywindow , 10, 2);
	wprintw(mywindow, "Trophy coord x: %d" , temp );

	//snake Y
	temp = mySnake.head_location_y;
	wmove(mywindow , 12, 2);
	wprintw(mywindow, "Snake coord  y: %d" , temp );

	//trophy Y
	temp = mytrophy.location.y;
	wmove(mywindow , 14, 2);
	wprintw(mywindow, "Trophy coord y: %d" , temp );

	//move the cursor out of the way
	wmove(mywindow , 0, 0);
}

int new_random_number(int min,  int max)
{

	int r;
	const unsigned int range = 1 + max - min;
	const unsigned int buckets = RAND_MAX / range;
	const unsigned int limit = buckets * range;
	static int is_initialized =FALSE;

	if(!is_initialized){
		time_t t ;
		/* Intializes random number generator */
		srand((unsigned) time(&t));
		is_initialized = TRUE;
	}

	/* Create equal size buckets all in a row, then fire randomly towards
	 * the buckets until you land in one of them. All buckets are equally
	 * likely. If you land off the end of the line of buckets, try again. */
	do
	{
		r = rand();
	} while (r >= limit);

	return min + (r / buckets);
}

int old_new_random_number(int lowest_val , int upper_val){

	int number ;
	int interval ;
	interval = abs(upper_val - lowest_val);

	number = rand();
	number = abs(number);
	number = number % (interval+1);
	number = number + lowest_val;
	return number ;
}

bool is_win_game(){

	if (mySnake.size>= mygame.win_score){
		return TRUE;
	}else {
		return FALSE ;
	}

}


void feed_snake(){

	int num_links = mytrophy.value;
	int i ;
	struct point location ;
	int x_direction  , y_direction ;


	//find the last link of the snake
	//struct point location = mySnake.links[mySnake.size -1];


	enum Enum_Direction tail_growth_direction ;



	//if the snake is one character long append to the opposite direction of the snake
	if (mySnake.size <=1){

		switch (mySnake.Direction){

		case Direction_up:
			tail_growth_direction =  Direction_down;

			break ;
		case Direction_down:
			tail_growth_direction =Direction_up;

			break ;
		case Direction_right:
			tail_growth_direction =Direction_left;

			break ;
		case Direction_left:
			tail_growth_direction =Direction_right;

			break ;
		}

	}else {
		//find the direction of the last two links
		//and append the new links to them


		//if x_direction ==0 the last two links are moving up or down
		//if x_direction >0 the tail is moving left other wise moving right
		x_direction =mySnake.links[mySnake.size-1].x -mySnake.links[mySnake.size-2].x;

		//if y_direction ==0 the last two links are moving right or left
		//if y_direction >0 the tail is moving up other wise moving down
		y_direction =mySnake.links[mySnake.size-1].y-mySnake.links[mySnake.size-2].y;
		//append links in the opposite direction of the tail
		//to avoid collisions

		if (x_direction > 0){
			tail_growth_direction = Direction_right;

		}else if (x_direction < 0){
			tail_growth_direction =Direction_left;

		}else if (y_direction > 0){
			tail_growth_direction =Direction_down;

		}else if (y_direction < 0){
			tail_growth_direction =Direction_up;

		}

	}








	for( i=0 ; i < num_links ; i++){
		switch(tail_growth_direction){

		case Direction_up:
			location.y --;
			mySnake.links[mySnake.size +i]= location;

			break ;
		case Direction_down:
			location.y++;
			mySnake.links[mySnake.size +i]= location;

			break ;
		case Direction_right:
			location.x++;
			mySnake.links[mySnake.size +i]= location;

			break ;
		case Direction_left:
			location.x--;
			mySnake.links[mySnake.size +i]= location;

			break ;




		}
	}
	//increase the snake size
	mySnake.size +=num_links;

}

bool is_collision_snake_with_trophy(){

	//check x collision
	if(mySnake.head_location_x != mytrophy.location.x){
		//x is not the same no collision
		return FALSE ;
	}

	//check y collision
	if(mySnake.head_location_y != mytrophy.location.y){
		//x is not the same no collision
		return FALSE ;
	}

	//the snake collided with the trophy
	return TRUE;

}

bool is_collision_snake_with_outline(){


	//check snake collision with left wall
	if ( mySnake.head_location_x <= 0){

		return TRUE ;
	}

	//check snake collision with right wall
	if ( mySnake.head_location_x >= mygame.window_max_x){

		return TRUE ;
	}

	//check snake collision with upper wall
	if ( mySnake.head_location_y <= 0){

		return TRUE ;
	}

	//check snake collision with lower wall
	if ( mySnake.head_location_y >= mygame.window_max_y){

		return TRUE ;
	}


	//no colision with the wall detected
	return FALSE ;
}

void init_game(){

	int window_height = getmaxy(mywindow) ;
	int window_width = getmaxx(mywindow);

	mygame.window_max_y =window_height;
	mygame.window_max_x = window_width;
	mygame.end_timer_interval_mSec = GAME_TIME_INTERVAL_END_SEC*1000;
	mygame.start_timer_interval_mSec = GAME_TIME_INTERVAL_START_SEC*1000;
	mygame.status = Game_Status_Running;
	mygame.seconds =0;
	mygame.milli_seconds=0;
	mygame.current_time_interval_sec = GAME_TIME_INTERVAL_START_SEC;

	//the win score is half the  parameter of the
	mygame.win_score = (window_height + window_width)/2;

	//this is the increments of the timer that animates the game. it is decreased as
	//the snake size grows
	mygame.increments_timer_intervals_mSec = (mygame.start_timer_interval_mSec -
			mygame.end_timer_interval_mSec )/mygame.win_score ;
}

void new_trophy(){





	int number;

	//trophy is initially on
	mytrophy.on_or_off = TRUE ;

	//calculate the time to live for the trophy
	number = new_random_number(TROPHY_MIN_TIME_TO_LIVE , TROPHY_MAX_TIME_TO_LIVE);
	mytrophy.time_to_live_secs = number + mygame.seconds;//when we need  new trophy

	// find trophy x coordinate
	number = new_random_number(TROPHY_MIN_DISTANCE_FROM_WALL ,
			mygame.window_max_x - TROPHY_MIN_DISTANCE_FROM_WALL);
	mytrophy.location.x = number;

	//find trophy y coordinate
	number = new_random_number(TROPHY_MIN_DISTANCE_FROM_WALL ,
			mygame.window_max_y - TROPHY_MIN_DISTANCE_FROM_WALL);
	mytrophy.location.y = number;


	//find the trophy value
	number = new_random_number(TROPHY_MIN_VALUE , TROPHY_MAX_VALUE);
	mytrophy.value = number ;




}

void init_window(){

	mywindow =	initscr();
	box( mywindow, ACS_VLINE, ACS_HLINE );
	//wmove(mywindow , 10 , 10);
	//wprintw(mywindow , "helloooooo");
	//mvwprintw( mywindow, 0, 0, "Hello World %d", 123 );
	refresh();			/* Print it on to the real screen */
	//getch();			/* Wait for user input */
	//endwin();


}

void redraw_trophy(){

	//turn tropy on and off
	if(mytrophy.on_or_off == FALSE){
		mytrophy.on_or_off = TRUE;
		return ;
	}

	//turn on the trophy
	wmove(mywindow , mytrophy.location.y , mytrophy.location.x);
	wprintw(mywindow , "%d" , mytrophy.value);
	wmove(mywindow , 0 , 0);

	//next time turn the trophy off
	mytrophy.on_or_off = FALSE;
}

bool is_collison_snake_with_self(){

	int i ;


	//check of the head of the snakes hits
	//other links of the snake
	for (i=1; i< mySnake.size;i++){

		if (mySnake.links[0].x == mySnake.links[i].x &&
				mySnake.links[0].y== mySnake.links[i].y	){
			return TRUE ;

		}
	}

	return FALSE ;


}

void cleanup_exit(){

	endwin();
	exit ;

}

void init_snake(){
	int x , y ;

	//the head of the snake is always
	//in the middle of the window
	x = mygame.window_max_x/2;
	y = mygame.window_max_y/2;
	mySnake.head_location_x = x;
	mySnake.head_location_y=y;

	//the direction of the snake is selected randomly
	//Randomly select oe of the four directions
	mySnake.Direction = new_random_number(0, 3);

	mySnake.size = SNAKE_INITIAL_SIZE ;
	int i ;
	x = mySnake.head_location_x;
	y =mySnake.head_location_y;
	for (i=0 ; i< mySnake.size;i++){
		mySnake.links[i].x=x;
		mySnake.links[i].y=y;
		//y--;
		x++;
	}



}


bool is_collision(){


	//check if the snakes hits himself
	if (is_collison_snake_with_self()){
		//disabled for debugginf
		return TRUE;
	}

	//check if the snakes hits the wall
	if (is_collision_snake_with_outline()){
		return TRUE ;
	}



	//no collisions to report
	return FALSE ;

}


void redraw_snake(){

	int i ;
	//print all the links of the snake






	for(i=0  ; i< mySnake.size ; i++){
		wmove(mywindow , mySnake.links[i].y , mySnake.links[i].x);
		wprintw(mywindow , SNAKE_LINK_CHARACTER);
		wmove(mywindow , 0, 0);
		//wprintw(mywindow , "printing i=%d, x=%d , y=%d" , i,mySnake.links[i].x , mySnake.links[i].y );

	}

}


void redraw_outline(){
	wclear(mywindow);
	box( mywindow, ACS_VLINE, ACS_HLINE );
}

void redraw_window(){





	//wmove(mywindow , mySnake.head_location_y , mySnake.head_location_x );
	//wprintw(mywindow , SNAKE_LINK_CHARACTER);


	//check the status of the game to see if it is stopped
	if (mygame.status == Game_Status_Lost){

		//draw a message box
		draw_message_box("you lost.");
		wgetch(mywindow);
		return ;



	}else if(mygame.status == Game_Status_win){

		//draw a message box
		draw_message_box("you won !!!");
		wgetch(mywindow);
		return ;
	}

	//draw outline
	redraw_outline();

	//draw snake
	redraw_snake();


	//draw trophy
	redraw_trophy();


	//redraw game data
	if (DISPLAY_GAME_DATE){
		redraw_game_data();
	}

	//debugging
	//wmove(mywindow , 10, 10);
	//wprintw(mywindow , "game=%d , trophy=%d" , mygame.seconds , mytrophy.time_to_live_secs);
	//wmove(mywindow , 0, 0);
	refresh();

}

void move_snake(){

	int i;

	switch (mySnake.Direction){
	case Direction_down :
		mySnake.head_location_y++;
		break ;

	case Direction_up :
		mySnake.head_location_y--;
		break ;

	case Direction_right :
		mySnake.head_location_x++;
		break ;
	case Direction_left :
		mySnake.head_location_x--;
		break ;


	}


	int size = mySnake.size ;
	//copy the next coordinates of all the link
	for (i=1 ;i< size ; i++){
		mySnake.links[size -i] = mySnake.links[size -i-1];
		//mySnake.links[size -i].x = mySnake.links[size -i-1].x;
		//mySnake.links[size -i].y = mySnake.links[size -i-1].y;
	}

	mySnake.links[0].x =mySnake.head_location_x;
	mySnake.links[0].y =mySnake.head_location_y;

}

void game_logic(){


	//don't update if thegame is stoped
	if ( mygame.status == Game_Status_Lost
			|| mygame.status==Game_Status_Stopped
			|| mygame.status == Game_Status_win){

		//just return don't do anything
		return ;
	}

	//check for collisions
	if (is_collision()){

		mygame.status = Game_Status_Lost ;
		return ;
	}


	//if if the play won
	if (is_win_game()){

		mygame.status = Game_Status_win;

	}


	//check for trophy collision
	if (is_collision_snake_with_trophy()){

		//feed the snake
		feed_snake();

		//increase the snake speed
		//decrease time interval proportional to the snake size
		mygame.current_time_interval_sec =mygame.start_timer_interval_mSec-
				mygame.increments_timer_intervals_mSec*mySnake.size;

		//convert from mSeconds to seconds
		mygame.current_time_interval_sec/=1000;


		//create a new trophy
		new_trophy();

	}


	//check if you need to create a new trophy
	//if the old one expired
	if (mygame.seconds >= mytrophy.time_to_live_secs){
		new_trophy();

	}



	//move the snake one step
	move_snake();

}

void init_timer(){
	int ret;

	//timer tick every 1 ms
	signal (SIGALRM, timer_handler);
	delay.it_value.tv_sec =0;
	delay.it_value.tv_usec = 1000;
	delay.it_interval.tv_sec = 0;
	delay.it_interval.tv_usec=0;

	//arm the timer
	ret = setitimer (ITIMER_REAL, &delay, NULL);
	if (ret) {
		perror ("setitimer");
		return;
	}

}

void timer_handler(){

	//this routine is called every 1mS
	static 	int tick_to_update_game=0;
	static int tick_to_next_second=0;
	tick_to_update_game ++;



	//check if we need to update the game;
	if (tick_to_update_game >= mygame.current_time_interval_sec*1000){
		game_logic();
		redraw_window();
		tick_to_update_game=0;
	}

	//one  minute passed, increment the minutes keeper
	if (tick_to_next_second >= 1000){
		mygame.seconds ++;
		tick_to_next_second=0;

		//reset the millisecond counter, we don't want any integer overflow
		mygame.milli_seconds=-1;
	}
	//Increment game milliseconds counter
	mygame.milli_seconds++;
	tick_to_next_second++;

	int ret;
	//re-arm the timer
	ret = setitimer (ITIMER_REAL, &delay, NULL);
	if (ret) {
		perror ("setitimer");
		return;
	}


}


WINDOW *create_newwin(int height, int width, int starty, int startx){
	WINDOW *local_win;
	local_win = newwin(height, width, starty, startx);
	box(local_win, 0 , 0); /* 0, 0 gives default characters
	 * for the vertical and horizontal
	 * lines */
	wrefresh(local_win); /* Show that box */
	return local_win;
}

char draw_message_box(char * some_message){


	WINDOW *my_win;
	int startx, starty, width, height;
	int ch;
	//initscr(); /* Start curses mode */
	//cbreak(); /* Line buffering disabled, Pass on
	//* everty thing to me */
	//keypad(stdscr, TRUE); /* I need that nifty F1 */
	height = 6;
	width = 16;
	starty = (LINES - height) / 2; /* Calculating for a center placement */
	startx = (COLS - width) / 2; /* of the window */
	//printw(some_message);

	my_win = create_newwin(height, width, starty, startx);
	move( mygame.window_max_y/2, mygame.window_max_x/2-6);
	printw(some_message);
	move( 0 ,0);
	refresh();
}
