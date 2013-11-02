/*
 * cthread tests program
 *
 * Copyright Antonio Barbalace, SSRG, VT, 2013
 */

#include <stdio.h>

#include "cthread.h"

fun_modula () {
	cthread_setaffinity_np ();
}

void test_modula() {
	cthread_create();
	cthread_join();

	cthread_create();
	cthread_join();

	cthread_create();
	cthread_join(); // in which way is done in OpenMP ?!
}

int main () {
  test_modula();
}
