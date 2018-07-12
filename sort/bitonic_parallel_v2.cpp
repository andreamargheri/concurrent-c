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
}

void* bitonic_comparator(void *arg){
	params* data = (params*)arg;
	int el1 = data-> estremoSx;
	int el2 = data-> estremoDx;
	//cout<< "       comparo elementi " << el1 << " e " << el2 << endl;		
	if (data->flag == true){
		//lista crescente
		if (data->listaI[el1] > data->listaI[el2]){
			//scambio elementi
			int swap = data->listaI[el1];
			data->listaI[el1] = data->listaI[el2];
			data->listaI[el2] = swap;
		}
	}else{
		//lista decrescente
		if (data->listaI[el1] < data->listaI[el2]){
			//scambio elementi
			int swap = data->listaI[el1];
			data->listaI[el1] = data->listaI[el2];
			data->listaI[el2] = swap;
		}
	}
	//pthread_exit(NULL);
}

void* bitonic_merge(void *arg){
	params* data = (params*)arg;

	pthread_t tid1[NUM_THREADS];
	params argoments1[NUM_THREADS];

	//cout<< "ordino in modo " << data->flag << " posizioni "<< data->estremoSx <<" - " << data->estremoDx<< endl;	
	//per sapere la distanza a cui applicare il comparator utilizzare n/2 n/4 n/8
	int dimTot = (data->estremoDx) - (data->estremoSx); 
	int x = 1;
	int gap = 0;
	int totOp = dimTot>>1;	
	while (gap != 1){
		x = x<<1;
		gap = dimTot/x;
		int n_groups = x>>1;
		//n_comp rappresenta il numero di operazioni in ogni gruppo
		int n_comp = totOp / n_groups;
		int estS = data->estremoSx;
		for (int i = 0; i <x/2; i++){
			int nThread;
			int remain_Comp = n_comp;
			while (remain_Comp >0){
				nThread = (n_comp<=NUM_THREADS)? n_comp : NUM_THREADS;
				for (int j = 0; j<nThread; j++){
					argoments1[j].flag = data->flag;							
					argoments1[j].listaI = data->listaI;
					argoments1[j].estremoSx = estS;
					argoments1[j].estremoDx = estS+gap;
					pthread_create(&tid1[j], NULL, &bitonic_comparator, &argoments1[j]);
					estS++;
				}
				//aspetto fine threads
				for (int j = 0; j<nThread; j++){	
					pthread_join(tid1[j], NULL);
				}
				//si passa al gruppo successivo quando ho esaurito le operazioni di quel gruppo
				remain_Comp = remain_Comp - nThread;
			}
			estS= estS+gap;
		}
		//cout << " fine stage distanza " << gap<<endl;
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
		
		//cout << "fine step"<< i <<endl;
	}	
}


