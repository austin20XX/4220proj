#ifndef JUKEBOX_4220_H
#define JUKEBOX_4220_H

#include <iostream>
#include <string>
#include <list>
#include <unistd.h>
#include <algorithm>
#include <ao/ao.h>
#include <mpg123.h>

namespace wiringPi {
#include <wiringPi.h>
}

#define BITS 8

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
	float getChange() const;
	bool addRequest(int, std::string);
	bool removeRequest(int, std::string);
	void addChange(int, int, int);
	bool refundChange(float);
	void printQueue() const;
	void printSongs() const;
	bool playNext();	
	enum button_t {
		song_select = 27, 
		refund = 0, //button 2
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

	const char* getNextPath();
	
}; //end class

const char* Jukebox::getNextPath(){
	auto n = song_queue.front();
	
	const char* path = n.song.path.c_str();

	song_queue.pop_front();

	return path;

}

bool Jukebox::playNext() {
	
	//code from https://hzqtc.github.io/2012/05/play-mp3-with-libmpg123-and-libao.html
	if(song_queue.empty()){
		return false;
	}

	mpg123_handle *mh;
	unsigned char *buffer;
	size_t buffer_size;
	size_t done;
	int err;

	int driver;
	ao_device *dev;

	ao_sample_format format;
	int channels, encoding;
	long rate;


	    /* initializations */
	ao_initialize();
	driver = ao_default_driver_id();
	mpg123_init();
	mh = mpg123_new(NULL, &err);
	buffer_size = mpg123_outblock(mh);
	buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));

	    /* open the file and get the decoding format */
	mpg123_open(mh, getNextPath());
	mpg123_getformat(mh, &rate, &channels, &encoding);

	    /* set the output format and open the output device */
	format.bits = mpg123_encsize(encoding) * BITS;
	format.rate = rate;
	format.channels = channels;
	format.byte_format = AO_FMT_NATIVE;
	format.matrix = 0;
	dev = ao_open_live(driver, &format, NULL);

	    /* decode and play */
	while (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK){
		ao_play(dev, (char*)buffer, done);
	}

		/* clean up */
	free(buffer);
	ao_close(dev);
	mpg123_close(mh);
	mpg123_delete(mh);
	mpg123_exit();
	ao_shutdown();
	return true;
}


//song accessor
std::list<song_t> Jukebox::getSongs() const {
 	return available_songs;
}

//Queue printer
void Jukebox::printSongs() const {
	int i=1;
	std::cout << "\nPlease select a song you would like to add to the queue" <<std::endl;
	for(auto song : available_songs) {
		std::cout << i << ": " << song.title << " by " << song.artist;
		std::cout << ". Cost: $ "<< song.price << std::endl;
		i++;
	}
	
}

void Jukebox::printQueue() const {
	int i = 1;
	std::cout << "\nThe current order of the song queue is: " << std::endl;
	for(auto selection : song_queue) {
		std::cout << i << ": " << selection.song.title << " by " << selection.song.artist;
		std::cout << ". Number of requests: " << selection.request_count <<std::endl;
	}	
}
//Amount of money input accessor
float Jukebox::getChange() const {
	return accumulator;
}

//Add a song to the queue
bool Jukebox::addRequest(int song_num, std::string password) {
	auto song = available_songs.begin();
	auto queue_iterator = song_queue.begin();
	int i=1;
	for(i; i < song_num; i++) {
		if(song==available_songs.end()){
			return false;
		}
		else{
			song++;
		}
	}
	
	if(song->price > accumulator){
		return false;
	}

	for(queue_iterator; queue_iterator != song_queue.end(); ++queue_iterator) {
		if(song->title.compare((queue_iterator->song).title) == 0 &&
			song->artist.compare((queue_iterator->song).artist) == 0 ) {
				(queue_iterator->passwords).push_back(password);
				(queue_iterator->request_count)++;
				song_queue.sort(queue_sort);
				accumulator = 0.0;
				return true;	
		}
	}

	song_selection new_select;
	new_select.song = *song;
	new_select.passwords.push_back(password);
	new_select.request_count++;

	song_queue.push_back(new_select);
	accumulator = 0.0;	
	return true;
}

bool Jukebox::removeRequest(int song_num, std::string password) {
	auto selection = song_queue.begin();

	int i=1;
	for(i; i < song_num; i++) {
		if(selection==song_queue.end()){
			return false;
		}
		else{
			selection++;
		}
	}
	
	auto pw_iterator = selection->passwords.begin();
	for(auto pw : selection->passwords) {
		if(password.compare(pw) == 0){
			(selection->request_count)--;
			song_queue.sort(queue_sort);
			//If the count got decreased to 0. Pop the last element since it should be at end after sort
			std::cout << "Refunding: " << selection->song.price << std::endl;	
			refundChange(selection->song.price);
			if(selection->request_count == 0) {
				song_queue.pop_back();	
			}
			return true;
		}
	}

	return false;
}

bool queue_sort(const song_selection& first, const song_selection& second) {
	if(first.request_count > second.request_count) {
		return true;
	}
	else if(first.request_count < second.request_count) {
		return false;
	}
	return true;
}

//return the change
bool Jukebox::refundChange(float amount) {
	float remaining = amount;
	while(remaining > 0 ) {
		if(coin_bank.quarters > 0 && remaining >= .25) {
			wiringPi::digitalWrite(yellowLed, HIGH);
			usleep(750000);
			wiringPi::digitalWrite(yellowLed, LOW);
			usleep(750000);

			remaining-=.25;
			coin_bank.quarters--;
			continue;
		}
		else if(coin_bank.dimes > 0 && remaining >= .10) {
			wiringPi::digitalWrite(greenLed, HIGH);
			usleep(750000);
			wiringPi::digitalWrite(greenLed, LOW);
			usleep(750000);
			
			remaining-=.10;
			coin_bank.dimes--;
			continue;
		}
		else if(coin_bank.nickels > 0 && remaining >= .05) {
			wiringPi::digitalWrite(blueLed, HIGH);
			usleep(750000);
			wiringPi::digitalWrite(blueLed, LOW);
			usleep(750000);
			
			remaining-=.05;
			coin_bank.nickels--;
			continue;
		}
		else if(remaining > 0 ) {
			//Unable to make change
			wiringPi::digitalWrite(redLed, HIGH);
			usleep(750000);
			wiringPi::digitalWrite(redLed, LOW);
			usleep(750000);

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
	wiringPi::pinMode(redLed, OUTPUT);

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

