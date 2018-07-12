#include <iostream>
#include <fstream>
#include <cmath>
#include <pthread.h>
#include <stdlib.h>
	
#define NUM_THREADS 4	
#define DIM 1024

using namespace std;

struct params {bool flag; unsigned int* listaI; int estremoSx; int estremoDx;};

void* bitonic_sort(unsigned int* lista);

int main (int argc, char** argv) {
	unsigned int lista[DIM];
	
	//read from file
	const char* input_file = argv[1];
	const char* output_file = argv[2];
	unsigned int data;	

	FILE* inputdata = fopen(input_file, "r");
	FILE* outputdata = fopen(output_file, "w");
	
	int i = 0;
	while(fread(&data, 1, sizeof(unsigned int), inputdata) != 0){
		lista[i] = data;
		i++;	
	}

	fclose(inputdata);

	//lancio l'ordinamento
	bitonic_sort(lista);

	//salvo dati ordinati
	for (i= 0; i<DIM; i++){
		fwrite(&lista[i], 1,sizeof(unsigned int), outputdata);
	}
	fclose(outputdata);

	pthread_exit(NULL);

}

int* bitonic_comparator(unsigned int* lista,int el1, int el2, bool flag){
	//cout<< "       comparo elementi " << el1 << " e " << el2 << endl;	
	if (flag == true){
		//lista crescente
		if (lista[el1] > lista[el2]){
			//scambio elementi
			int swap = lista[el1];
			lista[el1] = lista[el2];
			lista[el2] = swap;
		}
	}else{
		//lista decrescente
		if (lista[el1] < lista[el2]){
			//scambio elementi
			int swap = lista[el1];
			lista[el1] = lista[el2];
			lista[el2] = swap;
		}
	}
}

void* bitonic_merge(void *arg){
	params* data = (params*)arg;
	//cout<< "ordino in modo " << data->flag << " posizioni "<< data->estremoSx <<" - " << data->estremoDx<< endl;	
	//per sapere la distanza a cui applicare il comparator utilizzare n/2 n/4 n/8
	int dimTot = (data->estremoDx) - (data->estremoSx); 
	int x = 1;
	int div = 0;
	while (div != 1){
		x = x<<1;
		div = dimTot/x;
		int estS = data->estremoSx;
		for (int i = 0; i <x/2; i++){
			for (int j = 0; j<div; j++){
				bitonic_comparator(data->listaI,estS,estS+div,data->flag);
				estS++;
			}
			estS= estS+div;			
		}
	}
}

void* bitonic_sort(unsigned int* lista){
	pthread_t tid[NUM_THREADS];
	params argoments[NUM_THREADS];

	for (int i=1; i<=log2(DIM); i++){
		//definisco la dimensione dei pezzi da ordinare-> 2^num di stage corrente
		int dimC = 1<<i;
		//lancio l'esecuzione su pezzi di dimensione dimC
		//alterno flag per fare un blocco crescente e uno descrescente
		bool flag = true; //true=crescente
		// si valuta quanti thread si devono lanciare

		int num = DIM / dimC;
		
		//cout << "numero thread necessari" <<num << endl;
		
		int dimL = 0;
		//devo suddividere il lavoro tra i threads, un pezzo per uno
		int nThread;
		while (num>0){
			nThread = (num<=NUM_THREADS)? num : NUM_THREADS;
			for (int k = 0; k<nThread; k++){
				argoments[k].flag = flag;
				argoments[k].listaI = lista;
				argoments[k].estremoSx = dimL;
				argoments[k].estremoDx = dimL + dimC;
				pthread_create(&tid[k], NULL, &bitonic_merge, &argoments[k]);
				dimL = dimL + dimC;
				flag = !flag;
			}
			//aspetto fine threads
			for (int k = 0; k <nThread; k++){	
				pthread_join(tid[k], NULL);
			}
			num = num - nThread;
		}		
	}	
}


