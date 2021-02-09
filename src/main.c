/**
 * test.c
 * Small Hello World! example
 * to compile with gcc, run the following command
 * gcc -o test test.c -lulfius
 */
#include <stdio.h>
#include <string.h>
#include <ulfius.h>
#include <file/file.h>
#include <database/database.h>

#define NAME_LEN 120
#define ADDRESS_LEN 120
#define MAX_REGISTRIES 100

typedef struct
{
  int id;
  char name[NAME_LEN];
  char address[ADDRESS_LEN];
  unsigned char age;
} Person;

typedef struct
{
  Person person[MAX_REGISTRIES];
  int amount;
} Registries;

static Registries registries = {0};

#define PORT 8095

char *query(void *data)
{
  static char buffer[1024];

  Person *person = (Person *)data;

  memset(buffer, 0, 1024);

  snprintf(buffer, 1024, "insert into Person (name, address, age) values(\'%s\', \'%s\', %d)", person->name, person->address, person->age);

  return buffer;
}

/**
 * Callback function for the web application on /helloworld url call
 */
int callback_index(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  char *first = NULL;
  char *last = NULL;
  FILE_getFileContent("assets/pages/first.html", "r", &first);
  FILE_getFileContent("assets/pages/last.html", "r", &last);
  char *data = "<td>Cristiano Silva de Souza</td><td>Street 14</td><td>34</td>";
  size_t len = strlen(first) + strlen(last) + strlen(data);
  char *page = (char *)malloc(len + 1);
  snprintf(page, len, "%s%s%s", first, data, last);

  ulfius_set_string_body_response(response, 200, page);

  free(first);
  free(last);
  free(page);

  return U_CALLBACK_CONTINUE;
}

int callback_new(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  char *page = NULL;
  FILE_getFileContent("assets/pages/new.html", "r", &page);
  ulfius_set_string_body_response(response, 200, page);
  free(page);
  return U_CALLBACK_CONTINUE;
}

int callback_edit(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  ulfius_set_string_body_response(response, 200, "Edit Page!");
  return U_CALLBACK_CONTINUE;
}

int callback_insert(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  printf("POST parameter fname: %s\n", u_map_get(request->map_post_body, "fname"));
  printf("POST parameter faddress: %s\n", u_map_get(request->map_post_body, "faddress"));
  printf("POST parameter fage: %s\n", u_map_get(request->map_post_body, "fage"));

  Person person = {.id = 0};
  strncpy(person.name, u_map_get(request->map_post_body, "fname"), NAME_LEN);
  strncpy(person.address, u_map_get(request->map_post_body, "faddress"), ADDRESS_LEN);
  person.age = atoi(u_map_get(request->map_post_body, "fage"));

  Database_queryExec(query, &person);

  char *page = NULL;
  FILE_getFileContent("assets/pages/index.html", "r", &page);
  ulfius_set_string_body_response(response, 200, page);
  free(page);
  return U_CALLBACK_CONTINUE;
}

int callback_delete(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  printf("POST parameter.\n");
  ulfius_set_string_body_response(response, 200, "Ok");
  return U_CALLBACK_CONTINUE;
}

int callback_default(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  ulfius_set_string_body_response(response, 404, "Page not found, do what you want");
  return U_CALLBACK_CONTINUE;
}

/**
 * main function
 */
int main(void)
{
  struct _u_instance instance;

  Database_init("localhost", "root", "root", "Registry");

  // Initialize instance with the port number
  if (ulfius_init_instance(&instance, PORT, NULL, NULL) != U_OK)
  {
    fprintf(stderr, "Error ulfius_init_instance, abort\n");
    return (1);
  }

  u_map_put(instance.default_headers, "Access-Control-Allow-Origin", "*");

  // Maximum body size sent by the client is 1 Kb
  instance.max_post_body_size = 1024;
  instance.max_post_param_size = 1024;

  // Endpoint list declaration
  ulfius_add_endpoint_by_val(&instance, "GET", "/", NULL, 0, &callback_index, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", "/new", NULL, 0, &callback_new, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", "/edit", NULL, 0, &callback_edit, NULL);
  ulfius_add_endpoint_by_val(&instance, "POST", "/insert", NULL, 1, &callback_insert, NULL);
  ulfius_add_endpoint_by_val(&instance, "POST", "/delete", NULL, 0, &callback_delete, NULL);

  ulfius_set_default_endpoint(&instance, &callback_default, NULL);

  // Start the framework
  if (ulfius_start_framework(&instance) == U_OK)
  {
    printf("Start framework on port %d\n", instance.port);

    // Wait for the user to press <enter> on the console to quit the application
    getchar();
  }
  else
  {
    fprintf(stderr, "Error starting framework\n");
  }

  printf("End framework\n");
  ulfius_stop_framework(&instance);
  ulfius_clean_instance(&instance);
  Database_close();

  return 0;
}