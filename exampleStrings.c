/**
 *  
 * The student needs to compile, test and comment the source code file.
 * 
 * Enumerate those Rules and Recommendation associated that are broken in the previous source code file.
 * Enumerate he compilation tools and paramenters (gcc vs g++), -Wall ...
 * Enumerate the standard associated  -std=c99, -std=c11
 * 
 * There are several variants. You should choose at least two.
 * At the end the source code  should compile without warnings to the variant selected (you can remove/change instructions).
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
 
char array1[] = "Foo" "bar";
char array2[] = { 'F', 'o', 'o', 'b', 'a', 'r', '\0' };
 
enum { BUFFER_MAX_SIZE = 1024 };
 
const char* s1 ="\nHello\nWorld\n";
const char* s2 = "\nHello\nWorld\n";

void gets_example_func(void) {
  char buf[BUFFER_MAX_SIZE];
  char *p;

  if (fgets(buf, sizeof(buf), stdin)) {
    p = strchr(buf, '\n');
    if (p) {
      *p = '\0';
    }
  } else {
    return;
  }
  
}

char *get_dirname(const char *pathname, char *dirname, size_t size) 
{
  const char *slash;
  slash = strrchr(pathname, '/');
  if (slash) {
    ptrdiff_t slash_idx = slash - pathname;
    if((size_t)slash_idx < size) {
      memcpy(dirname, pathname, slash_idx); 
      dirname[slash_idx] = '\0'; 
      return dirname;
    }
  }
  return 0;
}
 

void get_y_or_n(void) {  
	char response[8];

	printf("Continue? [y] n: ");  
	fgets(response, sizeof(response), stdin);

	if (response[0] == 'n') 
		exit(0);  

	return;
}

 
int main(int argc, char *argv[])
{
  if (argc != 3) 
  {
	  printf("Error: Faltan argumentos.\n");
	  printf("Uso correcto: %s <argumento1> <argumento2>\n", argv[0]);
	  return 1;
  }

  char key[24];
  char response[8];
  char array3[16];
  char array4[16];
  char array5 []  = "01234567890123456";
  char ptr_char [] = "new string literal";
  // int size_array1 = strlen("аналитик");
  // int size_array2 = 100;
  
  // char analitic1[size_array1]="аналитик";
  // char analitic2[size_array2]="аналитик";
  // char analitic3[100]="аналитик";

  char dirname[260];
  if (get_dirname(__FILE__, dirname, sizeof(dirname))) { 
    puts(dirname); 
  }
      
  strcpy(key, argv[1]);  
  strcat(key, " = ");  
  strcat(key, argv[2]);


  fgets(response,sizeof(response),stdin);
  
  get_y_or_n();

  printf ("%s",array1);
  printf ("\n");
  printf ("%s",array2);
  printf ("\n");

  puts (s1);
  printf ("\n");
  puts (s2);
  printf ("\n");
  
  // truncamiento forzoso
  strncpy(array3, array5, sizeof(array3) -1); 
  array3[sizeof(array3) -1]  = '\0';

  // copia a array 4
  strncpy(array4, array3, strlen(array4) -1);
  array4[sizeof(array4) -1] = '\0'; 
  
  array5 [0] = 'M';
  ptr_char [0] = 'N';
  
  return 0;
}
