#include <iostream>
#include <fstream>
#include <cmath>
#include <pthread.h>
#include <stdlib.h>
	
#define NUM_THREADS 4	

using namespace std;

struct params {bool flag; unsigned int* listaI; int estremoSx; int estremoDx;int nMerge;int DIM;};

void* bitonic_sort(unsigned int* lista, int DIM);

int main (int argc, char** argv) {
	//clock_t       start;
	//double        elapsed;
	
	//start = clock();

	//read from file
	const char* input_file = argv[1];
	const char* output_file = argv[2];
	unsigned int data;	

	FILE* inputdata = fopen(input_file, "r");
	FILE* outputdata = fopen(output_file, "w");

	fseek ( inputdata , 0 , SEEK_END );
	int DIM = ftell (inputdata)/4;

	unsigned int* lista= new unsigned int [DIM];
	
	rewind(inputdata);	

	int i = 0;
	while(fread(&data, 1, sizeof(unsigned int), inputdata) != 0){
		lista[i] = data;
		i++;	
	}

	fclose(inputdata);

	//lancio l'ordinamento
	bitonic_sort(lista, DIM);

	//salvo dati ordinati
	for (i= 0; i<DIM; i++){
		fwrite(&lista[i], 1,sizeof(unsigned int), outputdata);
	}
	fclose(outputdata);

	//elapsed = (double)(clock()-start)/(double)CLOCKS_PER_SEC;
	//cout << "Elapsed time: " << elapsed << endl;
	pthread_exit(NULL);
	
}
//versione sequenziale
int* bitonic_comparator_seq(unsigned int* lista,int el1, int el2, bool flag){
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
//versione per thread
void* bitonic_comparator_thread(void *arg){
	params* data = (params*)arg;
	int nComp = data-> nMerge;
	//cout<< "       el1 " << data->estremoSx << " el2 " << data->estremoDx << endl;		
	for (int i= 0; i<nComp; i++){		
		int el1 = data-> estremoSx+i;
		int el2 = data-> estremoDx+i;
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
	}
	//pthread_exit(NULL);
}

void* bitonic_merge(void *arg){
	params* data = (params*)arg;

	params argoments1[NUM_THREADS];
	pthread_t tid1[NUM_THREADS];

	//per sapere la distanza a cui applicare il comparator utilizzare n/2 n/4 n/8
	int dimTot = (data->estremoDx) - (data->estremoSx); 
	int x = 1;
	int div = 0;
	for (int k=0; k<data->nMerge;k++){
		while (div != 1){
			x = x<<1;
			div = dimTot/x;
			int estS = data->estremoSx+k*dimTot;
			for (int i = 0; i <x/2; i++){
				//devo fare div comparatori in questo gruppo
				if (data->nMerge > 1 || div<(data->DIM/NUM_THREADS)){
					for (int j = 0; j<div; j++){
						bitonic_comparator_seq(data->listaI,estS,estS+div,data->flag);
						estS++;
					}
					estS= estS+div;	
				}else{
					int nComp;	
					int nThread;
					//cout << "da fare " << div << " comparator"<<endl;				
					if (div<=NUM_THREADS){
						nThread = div;
						nComp = 1;
					}else{
		 				nThread = NUM_THREADS;
						nComp = div/NUM_THREADS;
					}
					int divC = div;
					for (int r = 0; r<nThread; r++){
						if (r == nThread-1){
							//in caso di divisione con resto
							nComp = divC;
						}
						//cout << "lancio thread"<< endl;
						argoments1[r].flag = data->flag;						
						argoments1[r].listaI = data->listaI;						
						argoments1[r].estremoSx = estS;
						argoments1[r].estremoDx = estS+div;
						argoments1[r].nMerge = nComp;
						pthread_create(&tid1[r], NULL, &bitonic_comparator_thread, &argoments1[r]);
						estS=estS + 1*nComp;
						divC = divC - nComp;
					}
					estS= estS+div;	
					//aspetto fine threads
					for (int r = 0; r <nThread; r++){	
						pthread_join(tid1[r], NULL);
					}
				}
			}
		}
		data->flag = !data->flag;
		x = 1;
		div = 0;
	}
}

void* bitonic_sort(unsigned int* lista, int DIM){
	pthread_t tid[NUM_THREADS];
	params argoments[NUM_THREADS];

	for (int i=1; i<=log2(DIM); i++){
	
		//clock_t       start;
		//double        elapsed;
	
		//start = clock();

		//definisco la dimensione dei pezzi da ordinare-> 2^num di stage corrente
		int dimC = 1<<i;
		//lancio l'esecuzione su pezzi di dimensione dimC
		//alterno flag per fare un blocco crescente e uno descrescente
		bool flag = true; //true=crescente
		// si valuta quanti pezzi si devono controllare
		int num = DIM / dimC;
			
		int dimL = 0;
		//devo suddividere il lavoro tra i threads, più pezzi per uno se necessario
		int nThread;
		int nMerge;
		if (num<=NUM_THREADS){
			nThread = num;
			nMerge = 1;
		}else{
		 	nThread = NUM_THREADS;
			nMerge = num/NUM_THREADS;
		}
		for (int k = 0; k<nThread; k++){
			if (k == nThread -1){
				//in caso di divisione con resto
				nMerge = num;
			}
			argoments[k].flag = flag;
			argoments[k].listaI = lista;
			argoments[k].estremoSx = dimL;
			argoments[k].estremoDx = dimL + dimC;
			argoments[k].nMerge = nMerge;
			argoments[k].DIM = DIM;
			pthread_create(&tid[k], NULL, &bitonic_merge, &argoments[k]);
			dimL = dimL + dimC*nMerge;
			flag = !flag;
			num = num - nMerge;
		}
		//aspetto fine threads
		for (int k = 0; k <nThread; k++){	
			pthread_join(tid[k], NULL);
		}	
		//elapsed = (double)(clock()-start)/(double)CLOCKS_PER_SEC;
		//cout << "Elapsed time step "<< i << " : " << elapsed << endl;
			
		
	}	
}


