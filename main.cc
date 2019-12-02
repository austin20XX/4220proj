#include <iostream>
#include <mutex>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include "jukebox.h"


void *coin_thread_fn(void*);

//Mutex
std::mutex cout_mutex;
std::mutex quarter_mutex;
std::mutex dime_mutex;
std::mutex nickel_mutex;
std::mutex timestamp_mutex;
std::mutex busy_mutex;

//Globals
bool refund_pressed = false;
bool quarter_entered = false;
bool dime_entered = false;
bool nickel_entered = false; 

bool busy = false;
bool entering_coins = false; 

struct timespec last_action_timestamp;
Jukebox &jukebox = Jukebox::getInstance();



//ISRs
void quarter_isr(void);
void dime_isr(void);
void nickel_isr(void);

int main(void) {
	
	pthread_t coin_thread;
	pthread_t play_thread;

	jukebox.registerISR(Jukebox::quarter, &quarter_isr);
	jukebox.registerISR(Jukebox::dime, &dime_isr);
	jukebox.registerISR(Jukebox::nickel, &nickel_isr);

	pthread_create(&coin_thread, NULL, coin_thread_fn, NULL);

	pthread_exit(NULL);
}

void *coin_thread_fn(void* arg){

	int q, d , n;
	struct timespec cur_time;
	int time_difference;
	while(1){
		//Dont want to be able to input coins while refunding
		busy_mutex.lock();
		q = 0;
		d = 0; 
		n = 0;
		if(quarter_entered){
			entering_coins = true;
			q=1;
			jukebox.addChange(q, 0, 0);
			
			cout_mutex.lock();
			std::cout << "You've input: $" << jukebox.getChange() << std::endl;
			cout_mutex.unlock();

	
			quarter_mutex.lock();
			quarter_entered = false;
			sleep(1);
			quarter_mutex.unlock();
			
		}
		if(dime_entered){
			entering_coins = true;
			d=1;	
			jukebox.addChange(0, d, 0);
			
			cout_mutex.lock();
			std::cout << "You've input: $" << jukebox.getChange() << std::endl;
			cout_mutex.unlock();

			dime_mutex.lock();
			dime_entered = false;
			sleep(1);
			dime_mutex.unlock();

		}
		if(nickel_entered){
			entering_coins = true;
			n = 1;	
			jukebox.addChange(0, 0 , n);
			
			cout_mutex.lock();
			std::cout << "You've input: $" << jukebox.getChange() << std::endl;
			cout_mutex.unlock();

			nickel_mutex.lock();
			nickel_entered = false;
			sleep(1);
			nickel_mutex.unlock();
		}
		
		clock_gettime(CLOCK_REALTIME, &cur_time);
		timestamp_mutex.lock();
		time_difference = cur_time.tv_sec - last_action_timestamp.tv_sec;
		timestamp_mutex.unlock();  
		if(entering_coins == true && time_difference > 5) {
			entering_coins = false;
			
			cout_mutex.lock();
			std::cout << "Too much time elapsed... refunding $" << jukebox.getChange() << std::endl;
			cout_mutex.unlock();
			jukebox.refundChange(jukebox.getChange());

		}
	
		busy_mutex.unlock();
		usleep(300000);
	}//end while
pthread_testcancel();
} //end coin thread

void *refund_thread(void *arg) {
		

		
}

void quarter_isr() {
	if(quarter_mutex.try_lock()){
		quarter_entered = true;
		quarter_mutex.unlock();

		timestamp_mutex.lock();
		clock_gettime(CLOCK_REALTIME, &last_action_timestamp);
		timestamp_mutex.unlock();

	}
}

void dime_isr() {
	if(dime_mutex.try_lock()){
		dime_entered = true;
		dime_mutex.unlock();

		timestamp_mutex.lock();
		clock_gettime(CLOCK_REALTIME, &last_action_timestamp);
		timestamp_mutex.unlock();

	}
}

void nickel_isr() {
	if(nickel_mutex.try_lock()){
		nickel_entered = true;
		nickel_mutex.unlock();

		timestamp_mutex.lock();
		clock_gettime(CLOCK_REALTIME, &last_action_timestamp);
		timestamp_mutex.unlock();

	}
}
