/**
 * test.c
 * Small Hello World! example
 * to compile with gcc, run the following command
 * gcc -o test test.c -lulfius
 */
#include <stdio.h>
#include <string.h>
#include <ulfius.h>
// #include <file/file.h>

#define PORT 8095

char * print_map(const struct _u_map * map) {
  char * line, * to_return = NULL;
  const char **keys, * value;
  int len, i;
  if (map != NULL) {
    keys = u_map_enum_keys(map);
    for (i=0; keys[i] != NULL; i++) {
      value = u_map_get(map, keys[i]);
      len = snprintf(NULL, 0, "key is %s, value is %s", keys[i], value);
      line = o_malloc((len+1)*sizeof(char));
      snprintf(line, (len+1), "key is %s, value is %s", keys[i], value);
      if (to_return != NULL) {
        len = o_strlen(to_return) + o_strlen(line) + 1;
        to_return = o_realloc(to_return, (len+1)*sizeof(char));
        if (o_strlen(to_return) > 0) {
          strcat(to_return, "\n");
        }
      } else {
        to_return = o_malloc((o_strlen(line) + 1)*sizeof(char));
        to_return[0] = 0;
      }
      strcat(to_return, line);
      o_free(line);
    }
    return to_return;
  } else {
    return NULL;
  }
}

/**
 * Callback function for the web application on /helloworld url call
 */
// int callback_index(const struct _u_request *request, struct _u_response *response, void *user_data)
// {
//     char *page = NULL;
//     FILE_getFileContent("assets/pages/index.html", "r", &page);
//     ulfius_set_string_body_response(response, 200, page);
//     free(page);
//     return U_CALLBACK_CONTINUE;
// }

// int callback_new(const struct _u_request *request, struct _u_response *response, void *user_data)
// {
//     char *page = NULL;
//     FILE_getFileContent("assets/pages/new.html", "r", &page);
//     ulfius_set_string_body_response(response, 200, page);
//     free(page);
//     return U_CALLBACK_CONTINUE;
// }

int callback_edit(const struct _u_request *request, struct _u_response *response, void *user_data)
{
    ulfius_set_string_body_response(response, 200, "Edit Page!");
    return U_CALLBACK_CONTINUE;
}

int callback_insert(const struct _u_request *request, struct _u_response *response, void *user_data)
{
    char *url_params = print_map(request->map_url);
    char *headers = print_map(request->map_header);
    char *cookies = print_map(request->map_cookie);
    char *post_params = print_map(request->map_post_body);

    // printf("POST parameter fname: %s\n", u_map_get(request->map_post_body, "fname"));
    //   printf("POST parameter faddress: %s\n", u_map_get(request->map_post_body, "faddress"));
    //   printf("POST parameter fage: %s\n", u_map_get(request->map_post_body, "fage"));
    // char * post_params = print_map(request->map_post_body);
    // char * response_body = msprintf("Hello World!\n%s", post_params);
    // ulfius_set_string_body_response(response, 200, response_body);
    // o_free(response_body);
    // o_free(post_params);

    return U_CALLBACK_CONTINUE;
}

int callback_delete(const struct _u_request *request, struct _u_response *response, void *user_data)
{
    //   printf("POST parameter id: %s\n", u_map_get(request->map_post_body, "id"));
    printf("POST parameter.\n");
    ulfius_set_string_body_response(response, 200, "Ok");
    return U_CALLBACK_CONTINUE;
}

int callback_default (const struct _u_request * request, struct _u_response * response, void * user_data) {
  ulfius_set_string_body_response(response, 404, "Page not found, do what you want");
  return U_CALLBACK_CONTINUE;
}

/**
 * main function
 */
int main(void)
{
    struct _u_instance instance; 
    

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
    // ulfius_add_endpoint_by_val(&instance, "GET", "/", NULL, 0, &callback_index, NULL);
    // ulfius_add_endpoint_by_val(&instance, "GET", "/new", NULL, 0, &callback_new, NULL);
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

    return 0;
}