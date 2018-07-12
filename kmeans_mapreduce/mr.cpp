/*
 * mp.cpp
 *
 *   Author: Andrea Margheri
 */

#include "mr.h"
#include <iostream>
#include <fstream>
#include <omp.h>
#include <vector>
#include <algorithm>

static omp_lock_t maplock;
static omp_lock_t reducelock;
static std::ofstream* reduceOutput;
static std::vector<MR_pair*> interValues;		

static struct mySort {
  bool operator() (MR_pair* i,MR_pair* j) { return ((*i).k<(*j).k);}
} myOpSort;


//MAPPER
/**
 * out = output value on intermediate array
 */
void MR_emit(MR_pair* out){
	//emette valore intermedio risultato della funzione mapper
	//gestita la concorrenza con sem
	omp_set_lock(&maplock);
	interValues.push_back(out);
	omp_unset_lock(&maplock);
}

/**
 * s = start index input
 * e = end index input
 * input = ref input array
 * map = map function
 */
static void MR_map_wrapper(unsigned int s, unsigned int e, std::vector<MR_pair*>* input, MR_mapper map){
	//lancia varie map sulla porzione dell'input
	for(int i = s; i<e; i++){
		(map)((*input).at(i));
	}
}

//REDUCE
/**
 * value = output value
 */
void MR_output(void* value, MR_print print_reduce){
	//emette valori finale risultato della reduce
	omp_set_lock(&maplock);
	(*reduceOutput) << print_reduce(value) << std::endl;
	omp_unset_lock(&maplock);
}

//MASTER FUNCTION
void MR_mapreduce(unsigned int nThread, MR_mapper map, MR_reduce reduce, std::vector<MR_pair*>* input,unsigned int len, std::ofstream* output){
	//initialize
	omp_init_lock (&maplock);
	omp_init_lock (&reducelock);
	reduceOutput = output;
	(*reduceOutput) << "--------------" <<std::endl;	
	//dimensione wrap
	int dim = len / nThread;
	//lancio map_wrapper
	#pragma omp parallel for num_threads(nThread)
	for (int i=0;i<len;i=i+dim){
		//std::cout << i << std::endl;
		if ((i+dim)>=len){
			MR_map_wrapper(i,len,input,map);	
		}else{
			MR_map_wrapper(i,i+dim,input,map);	
		}
	}

	//risultati intermedi nel vettore interValues
	//---------Parte Sequenziale
	//sort by key
	sort(interValues.begin(), interValues.end(),myOpSort);
	//lancio reduce_wrapper
	std::vector<MR_pair*>::iterator it;
	//si lancia un wrapper per ogni chiave k
	//creo le liste per lanciare i thread
	std::vector< std::vector<void*>* > listValues;
	std::vector<int> listKey;
	std::vector<void*>* values = new std::vector<void*>();
	unsigned int key = interValues.at(0)->k;
	for ( it=interValues.begin(); it < interValues.end(); it++ ){
		MR_pair* pairR = *it;
		if (key == ((unsigned int)(*pairR).k)){
			//std::cout << key <<"add v " << p->x << std::endl;
			(*values).push_back((*pairR).v);
		}else{	
			//aggiungo liste all'elenco per lanciare le reduce
			listValues.push_back(values);
			listKey.push_back(key);
			//Inizio ricerca valori nuova chiave
			key = ((unsigned int)(*pairR).k);
			values = new std::vector<void*>();
			(*values).push_back((*pairR).v);
		}
	}
	//aggiungo ultima chiave
	listValues.push_back(values);
	listKey.push_back(key);
	//---------
	//lancio reduce
	/**
	 * key in listKey
	 * values in listValues
	 * reduce = reduce function
 	*/
	std::vector<std::vector<void*>*>::iterator itL;
	int i = 0;
	#pragma omp parallel for num_threads(nThread) 
	for ( itL=listValues.begin(); itL < listValues.end(); itL++ ){	
		reduce(listKey.at(i),(*itL));
		i = i +1;
	}
	(*reduceOutput) << "--------------" <<std::endl<<std::endl;	
}
