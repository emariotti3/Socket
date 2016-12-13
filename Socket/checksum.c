#include <stdio.h>
#include <math.h>

int calculate_checksum(char block[], int block_size){
	int lower = 0;
	int higher = 0;
	int ascii = 0;
	long int base = 2;
	long int pow_exp = 16;

	long m = pow(base,pow_exp);

	for (int i = 0; i < block_size; i++){
		ascii = (int) block[i];
		lower += ascii;
		higher += ascii * (block_size - i);
	}
	lower = lower % m;
	higher = higher % m;
	return lower + (higher * m);
}
