/*
 * CardTable.hpp
 *
 *  Created on: 2013-09-03
 *      Author: Tristan
 */

#ifndef _CARDTABLE_HPP_
#define _CARDTABLE_HPP_

#include "../Main/Object.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <climits>
#include "../defines.hpp"
#include <memory.h>
#include <string>
#include <sys/time.h> 
#include <math.h>

namespace traceFileSimulator {

class CardTable {

public:
	CardTable(int size,long heapSize);
	virtual ~CardTable();

    void freeCards();
    int  getCardIndex(long address);
    int  getCardSize();
    int  getNumCards();
	bool isCardMarked(long i);
    void markCard(long address);
    void markCards(long address,int numBits,char* bmap);
    long nextCardAddress(long address); //return address of the next card
    void syncCards8(char *bmap);
    void syncCards64(char *bmap); 
    void unmarkCards(long address,int numBits,char* bmap);
    void unmarkCard(long address);
   
private:
	int  cardSize;             //size of a card
	char *card;                //array representing the cards
	int  numCards;             //number of cards in the array 
    int  nShift;               //how many shift to perform based on card size
};

//These cards can be used separately or together
//Any class that includes CardTable.hpp can have access to these variables
extern CardTable* card1;  //tier 1, a card represents 8  bits of the bitmap
extern CardTable* card2;  //tier 2, a card represents 64  bits of the bitmap

} 
#endif /* _CARDTABLE_HPP_ */
