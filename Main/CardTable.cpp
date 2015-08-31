/*
 * CardTable.cpp
 *
 *  Created on: 2015-08-18
 *      Author: Tristan
 */

#include "CardTable.hpp"

using namespace std;

namespace traceFileSimulator {

CardTable* card1; //represents per 8 bits of the bitmap
CardTable* card2; //represents per 64 bits of the bitmap

CardTable::CardTable(int size,long heapSize) { //size is either 8 or 64
	cardSize = size;
    nShift   = (int)log2(size);                    //get how many shifts to perform based on cardSize
    numCards = (int)heapSize>>nShift;
    card     = (char*)calloc(numCards,sizeof(char));            //allocate and initialize cards to zero
    
}

int CardTable::getCardIndex(long address) { 
	return (int)address>>nShift;
}

int CardTable::getCardSize() {
	return cardSize;
}

int CardTable::getNumCards() { 
    return numCards;
}

bool CardTable::isCardMarked(long address) { 
	return (bool)card[address>>nShift] == (char)1;
}

void CardTable::markCard(long address) { 
	card[address>>nShift] = (char)1;
}

void CardTable::markCards(long address,int size,char* bmap) { //mark tier2 cards within the range [address, address + size]
    long i,end;

    i   = address>>nShift;
    end = (address+size)>>nShift;
    while (i <= end) {
        card[i] = (char)1;
        i++;
    }
}

long CardTable::nextCardAddress(long address) { 
	return (long)((address>>nShift)+1)<<nShift;
}

void CardTable::syncCards8(char *bmap) { //synchronize tier1 card table with the bitmap
    long i = 0;

    while (i<numCards) {
        if (bmap[i] == (char)MAX8BIT) 
           card[i] = (char)1;
        else
           card[i] = (char)0;
        i++;
    }
}

void CardTable::syncCards64(char *bmap) { //synchronize tier2 card table with the bitmap
    long i = 0;
    
    while (i<numCards) {
        if ((unsigned long long)&bmap[i<<3] == MAX64BIT) 
            card[i] = (char)1;
        else
            card[i] = (char)0;
        i++;
    }
}

void CardTable::unmarkCards(long address,int size,char* bmap) { //unmark cards within the range [address, address + size]
    long i,end;

    i   = address>>nShift;
    end = (address+size)>>nShift;
    while (i <= end)  {
        card[i] = (char)0;
        i++; 
    }
}

void CardTable::unmarkCard(long address) { 
	card[address>>nShift] = 0;
}

void CardTable::freeCards() { 
	free(card);
}

CardTable::~CardTable() {
}

}
