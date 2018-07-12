/*
 * mr.h
 *
 *      Author: Andrea Margheri
 */

#ifndef MR_H_
#define MR_H_

#include <vector>
#include <fstream>
#include <iostream>

typedef struct{
	unsigned int k;
	void* v;
}MR_pair;

//firma funzione che esegue il map
typedef void (*MR_mapper)(MR_pair* in);
//typedef void (*MR_mapper)();

//firma funzione che esegue la reduce
typedef void (*MR_reduce)(unsigned int k, std::vector<void*>* values);

//firma funzione per stampa valori in file da funzione MR_output
typedef std::string (*MR_print)(void* value);

void MR_mapreduce(unsigned int nThread, MR_mapper map, MR_reduce reduce,
		std::vector<MR_pair*>* input,unsigned int len,std::ofstream* output);

//funzioni per rilascio risultati in funzioni map e reduce
void MR_emit(MR_pair* out);

void MR_output(void* value, MR_print print_reduce);

#endif /* MR_H_ */

