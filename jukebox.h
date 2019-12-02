#ifndef JUKEBOX_4220_H
#define JUKEBOX_4220_H

#include <iostream>
#include <string>
#include <list>
#include <unistd.h>

namespace wiringPi {
#include <wiringPi.h>
}

typedef struct sawng {
	std::string title;
	std::string artist;
	std::string path;
	float price;
}song_t;

typedef struct sawng_select {
	song_t song;
	std::list<std::string> passwords;
	int request_count = 0;
}song_selection;

typedef struct piggy_bank {
	int quarters = 0;
	int dimes = 0;
	int nickels = 0;
} coin_store;

bool queue_sort(const song_selection&, const song_selection&);


class Jukebox {
    public:
        //Singleton design pattern
        static Jukebox& getInstance();

        //Destructor
        ~Jukebox();

	std::list<song_t> getSongs() const;
	std::list<song_selection> getQueue() const;
	float getChange() const;
	void addRequest(song_t, std::string);
	void addChange(int, int, int);
	bool refundChange(float);
	void printQueue() const;
	
	enum button_t {
		refund = 17, //button 2
		quarter = 1,
		dime = 24,
		nickel = 28,
	};

	enum led_t {
		redLed = 8,
		yellowLed = 9,
		greenLed = 7,
		blueLed = 21,
	};

        //Wrapper for registering wiringPi ISRs
        inline int registerISR(button_t b, void (*isr)()) {
            wiringPi::wiringPiISR(b, INT_EDGE_RISING,isr);
        }	

	
    private:
        //Default constructor is private so only access is through getInstance()
        Jukebox();

        //Delete copy constructor so can't create another by copying
        Jukebox(Jukebox const &) =  delete;
        void operator=(Jukebox const &) = delete;

	float accumulator;
	coin_store coin_bank;	
	
	std::list<song_t> available_songs;
	std::list<song_selection> song_queue;


	
}; //end class

//song accessor
std::list<song_t> Jukebox::getSongs() const {
 	return available_songs;
}
//Queue accessor
std::list<song_selection> Jukebox::getQueue() const {
	return song_queue;
}
//Amount of money input accessor
float Jukebox::getChange() const {
	return accumulator;
}

//Add a song to the queue
void Jukebox::addRequest(song_t req, std::string password) {
	auto queue_iterator = song_queue.begin();
	for(queue_iterator; queue_iterator != song_queue.end(); ++queue_iterator) {
		if(req.title.compare((queue_iterator->song).title) == 0 &&
			req.artist.compare((queue_iterator->song).artist) == 0 ) {
				(queue_iterator->passwords).push_back(password);
				(queue_iterator->request_count)++;
				song_queue.sort(queue_sort);
				return;	
		}
	}

	song_selection new_select;
	new_select.song = req;
	new_select.passwords.push_back(password);
	new_select.request_count++;

	song_queue.push_back(new_select);
	return;
}

bool queue_sort(const song_selection& first, const song_selection& second) {
	if(first.request_count > second.request_count) {
		return false;
	}
	else if(first.request_count < second.request_count) {
		return true;
	}
	return false;
}

//return the change
bool Jukebox::refundChange(float amount) {
	float remaining = amount;
	while(remaining > 0 ) {
		if(coin_bank.quarters > 0 && remaining >= .25) {
			wiringPi::digitalWrite(yellowLed, HIGH);
			usleep(500000);
			wiringPi::digitalWrite(yellowLed, LOW);
			usleep(500000);

			remaining-=.25;
			coin_bank.quarters--;
			continue;
		}
		else if(coin_bank.dimes > 0 && remaining >= .10) {
			wiringPi::digitalWrite(greenLed, HIGH);
			usleep(500000);
			wiringPi::digitalWrite(greenLed, LOW);
			usleep(500000);
			
			remaining-=.10;
			coin_bank.dimes--;
			continue;
		}
		else if(coin_bank.nickels > 0 && remaining >= .05) {
			wiringPi::digitalWrite(blueLed, HIGH);
			usleep(500000);
			wiringPi::digitalWrite(blueLed, LOW);
			usleep(500000);
			
			remaining-=.05;
			coin_bank.nickels--;
			continue;
		}
		else if(remaining > 0 ) {
			//Unable to make change
			wiringPi::digitalWrite(redLed, HIGH);
			usleep(500000);
			wiringPi::digitalWrite(redLed, LOW);
			usleep(500000);

			accumulator = 0.0;
			return false;
			
		}
	} //end while
	accumulator = 0.0;
	return true;	
} //end refund

void Jukebox::addChange(int num_quarters, int num_dimes, int num_nickels) {
	float total = (num_quarters*.25)+(num_dimes*.10)+(num_nickels*.05);
	
	accumulator+=total;

	coin_bank.quarters+=num_quarters;
	coin_bank.dimes+=num_dimes;
	coin_bank.nickels+=num_nickels;
}

//Default constructor
Jukebox::Jukebox() {
	song_t song1;
	song1.title = "Santa, Baby";
	song1.artist = "Eartha Kitt";
	song1.path = "./songs/santa_baby.mp3";
	song1.price = .75;


	song_t song2;
	song2.title = "Most Wonderful Time of the Year";
	song2.artist = "Andy Williams";
	song2.path = "./songs/most_wonderful_time.mp3";
	song2.price = .75;

	song_t song3;
	song3.title = "You're a Mean One, Mr. Grinch";
	song3.artist = "Thuri Ravenscroft";
	song3.path = "./songs/youre_a_mean_one.mp3";
	song3.price = 1.00;

	accumulator = 0.00;

	available_songs.push_back(song1);
	available_songs.push_back(song2);
	available_songs.push_back(song3);

	wiringPi::wiringPiSetup();

	wiringPi::pinMode(refund, INPUT);
	wiringPi::pinMode(quarter, INPUT);
	wiringPi::pinMode(dime, INPUT);
	wiringPi::pinMode(nickel, INPUT);
	
	wiringPi::pinMode(yellowLed, OUTPUT);
	wiringPi::pinMode(greenLed, OUTPUT);
	wiringPi::pinMode(blueLed, OUTPUT);

	wiringPi::digitalWrite(blueLed, LOW);
	wiringPi::digitalWrite(redLed, LOW);
	wiringPi::digitalWrite(yellowLed, LOW);
	wiringPi::digitalWrite(greenLed, LOW);
} //end default cons

//default destructor
Jukebox::~Jukebox() {
	wiringPi::pinMode(refund, INPUT);
	wiringPi::pinMode(quarter, INPUT);
	wiringPi::pinMode(dime, INPUT);
	wiringPi::pinMode(nickel, INPUT);

	wiringPi::pinMode(yellowLed, INPUT);
	wiringPi::pinMode(greenLed, INPUT);
	wiringPi::pinMode(blueLed, INPUT);

}


//Return static instance
Jukebox & Jukebox::getInstance() {
	static Jukebox instance;
	return instance;
}

#endif

