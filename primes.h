/*
 * primes.h
 *
 *  Created on: 20 марта 2015 г.
 *      Author: perepelitsyn
 */

#ifndef PRIMES_H_
#define PRIMES_H_

#include <stdint.h>

//typedef uintmax_t primes_t;
typedef uint32_t primes_t;

typedef struct PrimesList {
	primes_t count;
	primes_t range_start;
	primes_t range_end;
	primes_t *data;
	struct PrimesList *next;
} primes_list_t;

// возвращает количество простых чисел, меньших или равных заданному max_number
// опирается на предоставленный список простых чисел primes
primes_t get_primes_count(primes_list_t *primes, primes_t max_number);

// вычисляет целые числа из заданного диапазона [range_start, range_end],
// используя список делителей dividers.
// создает новый блок списка, помещает найденные числа туда,
// возвращает ссылку на этот список
primes_list_t *primes_calc(primes_t range_start, primes_t range_end, primes_list_t *dividers);

#endif /* PRIMES_H_ */
