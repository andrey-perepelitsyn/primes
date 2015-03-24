/*
 * primes.c
 *
 *  Created on: 20 марта 2015 г.
 *      Author: Andrey Perepelitsyn
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "primes.h"

// возвращает количество простых чисел, меньших или равных заданному
// опирается на предоставленный список простых чисел
primes_t get_primes_count(primes_list_t *primes, primes_t max_number) {
	primes_t count = 0, i_min, i_max;
	// пробежимся по блокам списка. остановимся на том, где последнее число больше заданного
	while(primes->data[primes->count - 1] <= max_number) {
		//fprintf(stderr, "%d\n", primes->count);
		if(primes->next == NULL) {
			// в предоставленном списке все числа меньше заданного. оставим это на совести вызывающего
			// и вернем общее количество чисел в списке
			//fprintf(stderr, "not enough dividers! returning %d\n", count + primes->count);
			return count + primes->count;
		}
		count += primes->count;
		primes = primes->next;
	}
	if(primes->data[0] > max_number)
		return count;
	// методом половинного деления найдем индекс наибольшего числа, которое меньше заданного
	i_min = 0;
	i_max = primes->count - 1;
	while(i_max - i_min > 1) {
		//fprintf(stderr, "[%3d,%3d]\n", i_min, i_max);
		primes_t i = (i_min + i_max) >> 1;
		if(primes->data[i] > max_number)
			i_max = i;
		else
			i_min = i;
	}
	return count + i_max;
}

// вычисляет целые числа из заданного диапазона, создает новый блок списка, помещает найденные числа туда
// возвращает ссылку на этот список
primes_list_t *primes_calc(primes_t range_start, primes_t range_end, primes_list_t *dividers) {
	primes_t i, j, n, dividers_count, max_divider;
	primes_t *counters;
	primes_list_t *div, *result;
	int n_is_prime;

	max_divider = (primes_t)sqrt(range_end);
	dividers_count = get_primes_count(dividers, max_divider);
	//fprintf(stderr, "primes_calc: [%d, %d], %d dividers:\n    ", range_start, range_end, dividers_count);
	//for(i = 0; i < dividers_count; i++)
		//fprintf(stderr, "%4d", dividers->data[i]);
	// так как все делители у нас в памяти, решето нам не нужно, его можно просто вообразить:)
	// а вот список под готовые простые числа надо сразу выделить:
	assert((result = (primes_list_t *)calloc(1, sizeof(primes_list_t))) != NULL);
	result->count = 0;
	result->range_start = range_start;
	result->range_end = range_end;
	// выделяем гарантированно достаточное количество памяти под найденные простые числа, используя известную формулу:
	// 0.89 * x / log(x) < п(x) < 1.11 * x / log(x)
	// добавим еще 20, чтобы скомпенсировать ошибку на малых значениях x
	i = 1.2 * range_end / log(range_end) - 0.8 * range_start / log(range_start + 1) + 20;
	result->data = (primes_t *)calloc(i, sizeof(primes_t));
	assert(result->data != NULL);
	//fprintf(stderr, "array of size %d has been allocated\n", i);
	result->next = NULL;
	// ищем простые числа из заданного диапазона в списке делителей и переписываем их в результаты
	if(range_start <= max_divider) {
		i = 0;
		div = dividers;
		while(div != NULL) {
			n = div->data[i++];
			if(i == div->count) {
				i = 0;
				div = div->next;
			}
			if(n > range_end)
				break;
			if(n >= range_start)
				result->data[result->count++] = n++;
		}
	}
	else
		n = range_start;
	// создаем массив счетчиков для хождения по решету, инициализируем остатками от деления n
	// если остаток == 0, пусть будет равен делителю
	counters = (primes_t *)calloc(dividers_count, sizeof(primes_t));
	assert(counters != NULL);
	//fprintf(stderr, "\ncounters:\n    ");
	for(i = j = 0, div = dividers; i < dividers_count; i++) {
		assert(div != NULL);
		counters[i] = n % div->data[j];
		if(counters[i] == 0)
			counters[i] = div->data[j];
		//fprintf(stderr, "%4d", counters[i]);
		if(++j == div->count) {
			j = 0;
			div = div->next;
		}
	}
	//fprintf(stderr, "\n");
	// проходим по просеиваемому диапазону
	for(; n <= range_end; n++) {
		// до проверки считаем n простым:
		n_is_prime = 1;
		//fprintf(stderr, "%3d ", n);
		// проходим по массиву счетчиков и списку делителей
		for(i = j = 0, div = dividers; i < dividers_count; i++) {
			// n делится на i-й делитель?
			if(counters[i]++ == div->data[j]) {
				counters[i] = 1;
				n_is_prime = 0;
			}
			if(++j == div->count) {
				j = 0;
				div = div->next;
			}
		}
		if(n_is_prime) {
			result->data[result->count++] = n;
			//fprintf(stderr, "%d is prime, count is %d\n", n, result->count);
		}
	}
	//fprintf(stderr, "%d primes found\n", result->count);
	// освобождаем память от счетчиков
	free(counters);
	// освобождаем лишнюю память
	result->data = (primes_t *)realloc(result->data, result->count * sizeof(primes_t));
	return result;
}
