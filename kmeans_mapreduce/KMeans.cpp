/*
 * KMeans.cpp
 *
 *   Author: Andrea Margheri
 */
#include <fstream>
#include <string.h>
#include <sstream>
#include <vector>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "mr.h"
#include <limits>

//parametri per algoritmo k-means
#define TOL 10
#define ITER 10
#define nThread 4

using namespace std;

typedef struct{
	double x;
	double y;
	string url;
}Photo;

typedef struct{
	double x;
	double y;
}Center;

static vector<Center*>* centroidi;

/*
*MR_pair=
*	x = indice della posizione del centroide in centroidi
*	v = Photo(x = lat photo; y = long photo; url)
*/
void mapper(MR_pair* in){
	//calcola distanza del punto da tutti i centroidi
	vector<Center*>::iterator it;	
	vector<double> allDist;
	Photo* p = (Photo*)(*in).v;
	for (it=(*centroidi).begin(); it < (*centroidi).end(); it++ ){
		double dist = sqrt(pow(((*p).x - (*it)->x),2) + pow(((*p).y - (*it)->y),2));
		allDist.push_back(dist);
	}
	//nuovo centroide asseganto al punto
	vector<double>::iterator itD;	
	double min = numeric_limits<double>::max();
	unsigned int id = 0;
	unsigned int i = 0;
	for (itD=allDist.begin(); itD < allDist.end(); itD++ ){
		if ((*itD)<min){
			min = (*itD);
			id = i;
		}		
		i = i+1;
	}
	in->k = id;
	MR_emit(in);
}

std::string print_reduce(void* value){
	Center* c = (Center*)value;
	std::ostringstream s;
	s << "Centroide (" << c->x << " , " << c->y << ")"<<std::endl;
	return s.str();
}

void reduce(unsigned int k,std::vector<void*>* values){
	//si aggiorna la posizione del centroide			
	//k = indice del centroide
	//values = lista di coordinate dei punti associati a k
	std::vector<void*>::iterator it;
	int len = 0;
	//aggiorno i valori dei centroidi
	double totX = 0;
	double totY = 0;
	unsigned int num = 0;
	for ( it=(*values).begin(); it < (*values).end(); it++ ){
		Photo* p = (Photo*)(*it);
		totX = totX + (*p).x;
		totY = totY + (*p).y;
		num = num + 1;
	}
	(*centroidi).at(k)->x = totX / num;
	(*centroidi).at(k)->y = totY / num;
	MR_output((void*)(*centroidi).at(k),(MR_print)print_reduce);
}

int main (int argc, char** argv){
	if (argc < 4){
		std::cout << std::endl;
		std::cout << "Numero argomenti non valido. Richiesto";
		std::cout << " -nCentroidi --nomeFileInput -nomeFileOutput"; 
		std::cout << std::endl;
	}else{
	//seed rand
	srand(time(NULL));

	//argomenti
	int nCentroidi = atof(argv[1]);
	const char* fileInput = argv[2];
	const char* fileOutput = argv[3];

	//int nThread = atof(argv[2]);	
	//int iter = atof(argv[4]);


	//cout.setf(ios::fixed, ios::floatfield);
	//cout.precision(8); 

	//carico i dati gps delle foto
	ifstream myReadFile;
	myReadFile.open(fileInput);

	string input;

	if (myReadFile.is_open()) {
		unsigned int len = 0;
		vector<MR_pair*>* vectorInput = new vector<MR_pair*>();		
		//si esclude prima riga
	    	getline(myReadFile,input);
	    	MR_pair* pair; 
		Photo* item;
		//for(int g = 0 ; g <10; g++){
		while (!myReadFile.eof()) {
		    	getline(myReadFile,input);
	 	    	stringstream os(input);  
			pair = new MR_pair();
		    	//key	
                    	pair->k = (rand()%nCentroidi);	
		    	//dati
			item = new Photo();
			os >> item->x;
			os >> item->y;
			os >> item->url;		    	
			pair->v = item;
			(*vectorInput).push_back(pair);
			len = len +1;
		}
		myReadFile.close();
		std::cout << "Fine lettura file dati gps." << std::endl <<"Da analizzare "<< len << " foto..." << std::endl;
		//creo i centroidi
		//scelgo punti a caso tra i punti gps delle foto
		centroidi = new vector<Center*>();
		for (int i=0; i<nCentroidi; i++){
			int pos = rand()%len;
			Photo* p = (Photo*)((*vectorInput).at(pos)->v);
			Center* c = new Center();
			c->x = (*p).x;
			c->y = (*p).y;
			(*centroidi).push_back(c);
		}
		vector<Center*>::iterator itC;	

		//file di output con i centroidi
		ofstream outputCentroidi;
		outputCentroidi.open("centroidi.data");
		//condizione arresto
		int* numPuntiC = new int[nCentroidi];
		int* numPuntiC_copy = new int[nCentroidi];
		for (int j=0; j<nCentroidi; j++){
			numPuntiC[j] = 0;
			numPuntiC_copy[j] = 0;
		}
		bool breakIter = false;
		int i = 0;

		std::vector<MR_pair*>::iterator itCe;
		while(i<ITER && !breakIter){			
			//Inizio MapReduce
			MR_mapreduce(nThread,(MR_mapper)mapper,(MR_reduce)reduce,vectorInput,len,&outputCentroidi);		
			//conto num punti associati a ogni centroide			
			for ( itCe=(*vectorInput).begin(); itCe < (*vectorInput).end(); itCe++ ){
				numPuntiC[(*itCe)->k] = numPuntiC[(*itCe)->k] +1;
			}
			//controllo condizione arresto algoritmo
			breakIter = true;
			for(int j=0; j<nCentroidi; j++){
				if (abs(numPuntiC[j]-numPuntiC_copy[j])<=TOL){
					breakIter = breakIter && true;
				}else{
					breakIter = breakIter && false;
				}
				numPuntiC_copy[j] = numPuntiC[j];
				numPuntiC[j] = 0;
			}
			i = i + 1;
		}
		outputCentroidi.close();

		//stampo su file i centroidi corrispondenti alle foto
		ofstream outputData;
		outputData.open(fileOutput);
		
		std::vector<MR_pair*>::iterator itI;
		for ( itI=(*vectorInput).begin(); itI < (*vectorInput).end(); itI++ ){
			outputData << (*itI)->k << std::endl;		
		}	
		outputData.close();

		std::cout << "...terminata analisi dopo "<< i << " iterazioni" << std::endl;

		if (breakIter){
			std::cout << "Iterazione terminata per successo condizione di arresto dopo "<< i << " iterazioni" << std:: endl;
		}
	}else{
		std::cout<< "Impossibile aprire il file dati" << endl;		
	}
	}
}
