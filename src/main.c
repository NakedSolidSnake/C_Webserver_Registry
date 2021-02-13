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
#include <collection/list_static.h>
#include <json/json.h>

#define NAME_LEN        120
#define ADDRESS_LEN     120
#define MAX_REGISTRIES  100

typedef struct 
{
    char *hostname;
    char *username;
    char *password;
    char *database;
} Database;

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
static List *static_list;

#define PORT 8095

static void loadParams(const char *properties, Database *database);

void Select_get(char **data, int rows, void *user_data)
{
  List *static_list = (List *)user_data;
  Person person;
  person.id = atoi(data[0]);  
  strncpy(person.address, data[1], ADDRESS_LEN);
  strncpy(person.name, data[2], NAME_LEN);
  person.age = atoi(data[3]);

  List_add(static_list, &person);
}

char *select_query(void *data)
{
  static char buffer[1024];

  memset(buffer, 0, 1024);

  snprintf(buffer, 1024, "select * from Person");

  return buffer;
}


char *insert_query(void *data)
{
  static char buffer[1024];

  Person *person = (Person *)data;

  memset(buffer, 0, 1024);

  snprintf(buffer, 1024, "insert into Person (name, address, age) values(\'%s\', \'%s\', %d)", person->name, person->address, person->age);

  return buffer;
}

char *delete_query(void *data)
{  
  int id = *(int *)data;
  static char buffer[1024] = {0};  

  snprintf(buffer, 1024, "delete from Person where id = %d", id);

  return buffer;
}

/**
 * Callback function for the web application on /helloworld url call
 */
int callback_index(const struct _u_request *request, struct _u_response *response, void *user_data)
{  
  char *page = NULL;
  FILE_getFileContent("assets/pages/index.html", "r", &page);
  ulfius_set_string_body_response(response, 200, page);
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
  Person person = {.id = 0};
  strncpy(person.name, u_map_get(request->map_post_body, "fname"), NAME_LEN);
  strncpy(person.address, u_map_get(request->map_post_body, "faddress"), ADDRESS_LEN);
  person.age = atoi(u_map_get(request->map_post_body, "fage"));

  Database_queryExec(insert_query, &person);

  char *page = NULL;
  FILE_getFileContent("assets/pages/index.html", "r", &page);
  ulfius_set_string_body_response(response, 200, page);
  free(page);
  return U_CALLBACK_CONTINUE;
}

int callback_delete(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  printf("POST parameter ID: %s.\n", u_map_get(request->map_post_body, "id"));
  int id = atoi(u_map_get(request->map_post_body, "id"));
  Database_queryExec(delete_query, &id);

  char *page = NULL;
  FILE_getFileContent("assets/pages/index.html", "r", &page);
  ulfius_set_string_body_response(response, 200, page);
  free(page);
  
  return U_CALLBACK_CONTINUE;
}

int callback_send_data(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  int dummy;
  char format[256];
  char data[8192] = {0};
  Database_queryExec(select_query, &dummy);
  List_clear(static_list);
  Database_resultSet(Select_get, static_list);

  for( int i = 0; i < List_size(static_list); i++)
  {
    Person person;
    List_getObjectByIndex(static_list, i, &person);
    memset(format, 0, sizeof(format));
    snprintf(format, sizeof(format), "%d,%s,%s,%d;", person.id, person.name, person.address, person.age);
    strncat(data, format, sizeof(data));
  }
  
  ulfius_set_string_body_response(response, 200, data);
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

  Database database = {0};

  loadParams("properties/properties.json", &database);

  static_list = List_create(100, sizeof(Person));

  // Database_init("localhost", "root", "root", "Registry");
  Database_init(database.hostname, database.username, database.password, database.database);

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
  ulfius_add_endpoint_by_val(&instance, "GET", "/send", NULL, 0, &callback_send_data, NULL);
  

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
  List_destroy(static_list);
  ulfius_stop_framework(&instance);
  ulfius_clean_instance(&instance);
  Database_close();

  return 0;
}

static void loadParams(const char *properties, Database *database)
{
    char buffer[1024] = {0};
    IHandler iParams[] = 
    {
        {.token = "hostname", .data = &database->hostname, .type = eType_String, .child = NULL},
        {.token = "username", .data = &database->username, .type = eType_String, .child = NULL},
        {.token = "password", .data = &database->password, .type = eType_String, .child = NULL},
        {.token = "database", .data = &database->database, .type = eType_String, .child = NULL}
    };

    IHandler iDatabase[] = 
    {
        {.token = "database", .data = NULL, .type = eType_Object, .child = iParams, .size = getItems(iParams)}
    };

    if(!getJsonFromFile(properties, buffer, 1024)){
        exit(EXIT_FAILURE);
    }

    processJson(buffer, iDatabase, getItems(iDatabase));
}