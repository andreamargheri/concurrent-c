#include <iostream>
#include <fstream>
#include <cmath>

#define DIM 1024

using namespace std;
int bitonic_sort(unsigned int* lista);

int main (int argc, char** argv) {
	unsigned int lista[DIM];
	//clock_t       start;
	//double        elapsed;
	
	//start = clock();

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
	
	//elapsed = (double)(clock()-start);
	//cout << "Elapsed time: " << elapsed << endl;
}

int* bitonic_comparator(unsigned int* lista,int el1, int el2, bool flag){
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

int* bitonic_merge(bool flag, unsigned int* lista, int estremoSx, int estremoDx){
	//per sapere la distanza a cui applicare il comparator utilizzare n/2 n/4 n/8
	int dimTot = estremoDx - estremoSx; 
	int x = 1;
	int div = 0;
	while (div != 1){
		x = x<<1;
		div = dimTot/x;
		int estS = estremoSx;
		for (int i = 0; i <x/2; i++){
			for (int j = 0; j<div; j++){
				bitonic_comparator(lista,estS,estS+div,flag);
				estS++;
			}
			estS= estS+div;			
		}
	}
}

int bitonic_sort(unsigned int* lista){
	for (int i=1; i<=log2(DIM); i++){
		//definisco la dimensione dei pezzi da ordinare
		//2^num di stage corrente
		int dimC = 1<<i;
		//lancio l'esecuzione su pezzi di dimensione dimC
		//alterno flag per fare n blocco crescente e uno descrescente
		bool flag = true; //ordino il pezzo in modo crescente
		for (int dimL = 0; dimL<DIM; dimL = dimL + dimC){
			//si passano le posizione del vettore entro le quali ordinare
			bitonic_merge(flag, lista, dimL, dimL + dimC);
			flag = !flag;		
			//si lavora sempre sullo stesso vettore
		}
	}

}


