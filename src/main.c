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
#include <zlib.h>
#include <pthread.h>

#define NAME_LEN        120
#define ADDRESS_LEN     120
#define MAX_REGISTRIES  100
#define PREFIX_STATIC "/assets"

#define U_COMPRESS_NONE 0
#define U_COMPRESS_GZIP 1
#define U_COMPRESS_DEFL 2

#define U_ACCEPT_HEADER  "Accept-Encoding"
#define U_CONTENT_HEADER "Content-Encoding"

#define U_ACCEPT_GZIP    "gzip"
#define U_ACCEPT_DEFLATE "deflate"

#define U_GZIP_WINDOW_BITS 15
#define U_GZIP_ENCODING    16

#define _U_W_BLOCK_SIZE 256

#define CHUNK 0x4000

struct _u_compressed_inmemory_website_config {
  char          * files_path;
  char          * url_prefix;
  struct _u_map   mime_types;
  char **         mime_types_compressed;
  size_t          mime_types_compressed_size;
  struct _u_map   map_header;
  char          * redirect_on_404;
  int             allow_gzip;
  int             allow_deflate;
  int             allow_cache_compressed;
  pthread_mutex_t lock;
  struct _u_map   gzip_files;
  struct _u_map   deflate_files;
};

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

static void * u_zalloc(void * q, unsigned n, unsigned m) {
  (void)q;
  return o_malloc((size_t) n * m);
}

static void u_zfree(void *q, void *p) {
  (void)q;
  o_free(p);
}

static const char * get_filename_ext(const char *path) {
    const char *dot = strrchr(path, '.');
    if(!dot || dot == path) return "*";
    if (strchr(dot, '?') != NULL) {
      *strchr(dot, '?') = '\0';
    }
    return dot;
}

/**
 * Streaming callback function to ease sending large files
 */
static ssize_t callback_static_file_uncompressed_stream(void * cls, uint64_t pos, char * buf, size_t max) {
  (void)(pos);
  if (cls != NULL) {
    return fread (buf, sizeof(char), max, (FILE *)cls);
  } else {
    return U_STREAM_END;
  }
}

/**
 * Cleanup FILE* structure when streaming is complete
 */
static void callback_static_file_uncompressed_stream_free(void * cls) {
  if (cls != NULL) {
    fclose((FILE *)cls);
  }
}

/**
 * static file callback endpoint
 */
static int callback_static_file_uncompressed (const struct _u_request * request, struct _u_response * response, void * user_data) {
  size_t length;
  FILE * f;
  char * file_requested, * file_path, * url_dup_save;
  const char * content_type;
  int ret = U_CALLBACK_CONTINUE;

  if (user_data != NULL && ((struct _u_compressed_inmemory_website_config *)user_data)->files_path != NULL) {
    file_requested = o_strdup(request->http_url);
    url_dup_save = file_requested;

    while (file_requested[0] == '/') {
      file_requested++;
    }
    file_requested += o_strlen(((struct _u_compressed_inmemory_website_config *)user_data)->url_prefix);
    while (file_requested[0] == '/') {
      file_requested++;
    }

    if (strchr(file_requested, '#') != NULL) {
      *strchr(file_requested, '#') = '\0';
    }

    if (strchr(file_requested, '?') != NULL) {
      *strchr(file_requested, '?') = '\0';
    }

    if (file_requested == NULL || o_strlen(file_requested) == 0 || 0 == o_strcmp("/", file_requested)) {
      o_free(url_dup_save);
      url_dup_save = file_requested = o_strdup("index.html");
    }

    file_path = msprintf("%s/%s", ((struct _u_compressed_inmemory_website_config *)user_data)->files_path, file_requested);

    if (access(file_path, F_OK) != -1) {
      f = fopen (file_path, "rb");
      if (f) {
        fseek (f, 0, SEEK_END);
        length = ftell (f);
        fseek (f, 0, SEEK_SET);

        content_type = u_map_get_case(&((struct _u_compressed_inmemory_website_config *)user_data)->mime_types, get_filename_ext(file_requested));
        if (content_type == NULL) {
          content_type = u_map_get(&((struct _u_compressed_inmemory_website_config *)user_data)->mime_types, "*");
        }
        u_map_put(response->map_header, "Content-Type", content_type);
        u_map_copy_into(response->map_header, &((struct _u_compressed_inmemory_website_config *)user_data)->map_header);

        if (ulfius_set_stream_response(response, 200, callback_static_file_uncompressed_stream, callback_static_file_uncompressed_stream_free, length, CHUNK, f) != U_OK) {
        }
      }
    } else {
      if (((struct _u_compressed_inmemory_website_config *)user_data)->redirect_on_404 == NULL) {
        ret = U_CALLBACK_IGNORE;
      } else {
        ulfius_add_header_to_response(response, "Location", ((struct _u_compressed_inmemory_website_config *)user_data)->redirect_on_404);
        response->status = 302;
      }
    }
    o_free(file_path);
    o_free(url_dup_save);
  } else {
    ret = U_CALLBACK_ERROR;
  }
  return ret;
}

int u_init_compressed_inmemory_website_config(struct _u_compressed_inmemory_website_config * config) {
  int ret = U_OK;
  pthread_mutexattr_t mutexattr;

  if (config != NULL) {
    config->files_path                 = NULL;
    config->url_prefix                 = NULL;
    config->redirect_on_404            = NULL;
    config->allow_gzip                 = 1;
    config->allow_deflate              = 1;
    config->mime_types_compressed      = NULL;
    config->mime_types_compressed_size = 0;
    config->allow_cache_compressed     = 1;
    if ((ret = u_map_init(&(config->mime_types))) != U_OK) {
    } else if ((ret = u_map_init(&(config->map_header))) != U_OK) {
    } else if ((ret = u_map_init(&(config->gzip_files))) != U_OK) {
    } else if ((ret = u_map_init(&(config->deflate_files))) != U_OK) {
    } else {
      pthread_mutexattr_init (&mutexattr);
      pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
      if (pthread_mutex_init(&(config->lock), &mutexattr) != 0) {
        ret = U_ERROR;
      }
    }
  }
  return ret;
}

void u_clean_compressed_inmemory_website_config(struct _u_compressed_inmemory_website_config * config) {
  if (config != NULL) {
    u_map_clean(&(config->mime_types));
    u_map_clean(&(config->map_header));
    u_map_clean(&(config->gzip_files));
    u_map_clean(&(config->deflate_files));
    free_string_array(config->mime_types_compressed);
    pthread_mutex_destroy(&(config->lock));
  }
}

int u_add_mime_types_compressed(struct _u_compressed_inmemory_website_config * config, const char * mime_type) {
  int ret;
  if (config != NULL && o_strlen(mime_type)) {
    if ((config->mime_types_compressed = o_realloc(config->mime_types_compressed, (config->mime_types_compressed_size+2)*sizeof(char*))) != NULL) {
      config->mime_types_compressed[config->mime_types_compressed_size] = o_strdup(mime_type);
      config->mime_types_compressed[config->mime_types_compressed_size+1] = NULL;
      config->mime_types_compressed_size++;
      ret = U_OK;
    } else {
      ret = U_ERROR;
    }
  } else {
    ret = U_ERROR_PARAMS;
  }
  return ret;
}

static void loadParams(const char *properties, Database *database);
int u_init_compressed_inmemory_website_config(struct _u_compressed_inmemory_website_config * config);

void u_clean_compressed_inmemory_website_config(struct _u_compressed_inmemory_website_config * config);

int u_add_mime_types_compressed(struct _u_compressed_inmemory_website_config * config, const char * mime_type);

int callback_static_compressed_inmemory_website (const struct _u_request * request, struct _u_response * response, void * user_data);

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

char *update_query(void *data)
{
  static char buffer[1024];

  Person *person = (Person *)data;

  memset(buffer, 0, 1024);

  snprintf(buffer, 1024, "update Person set name=\'%s\', address=\'%s\', age=%d where id = %d", person->name, person->address, person->age, person->id);

  return buffer;
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

  memset(buffer, 0, 1024);

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
  char *page = NULL;
  FILE_getFileContent("assets/pages/edit.html", "r", &page);
  ulfius_set_string_body_response(response, 200, page);
  free(page);

  return U_CALLBACK_CONTINUE;
}

int callback_update(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  Person person;
  person.id = person.age = atoi(u_map_get(request->map_post_body, "id"));
  strncpy(person.name, u_map_get(request->map_post_body, "fname"), NAME_LEN);
  strncpy(person.address, u_map_get(request->map_post_body, "faddress"), ADDRESS_LEN);
  person.age = atoi(u_map_get(request->map_post_body, "fage"));

  Database_queryExec(update_query, &person);

  ulfius_set_string_body_response(response, 200, "Update");
  return U_CALLBACK_CONTINUE;
}

int callback_insert(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  Person person = {.id = 0};
  strncpy(person.name, u_map_get(request->map_post_body, "fname"), NAME_LEN);
  strncpy(person.address, u_map_get(request->map_post_body, "faddress"), ADDRESS_LEN);
  person.age = atoi(u_map_get(request->map_post_body, "fage"));

  Database_queryExec(insert_query, &person);
  ulfius_set_string_body_response(response, 200, "");

  // char *page = NULL;
  // FILE_getFileContent("assets/pages/index.html", "r", &page);
  // ulfius_set_string_body_response(response, 200, page);
  // free(page);
  return U_CALLBACK_CONTINUE;
}

int callback_delete(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  int id = atoi(u_map_get(request->map_post_body, "id"));
  Database_queryExec(delete_query, &id);
  ulfius_set_string_body_response(response, 200, "");

  // char *page = NULL;
  // FILE_getFileContent("assets/pages/index.html", "r", &page);
  // ulfius_set_string_body_response(response, 200, page);
  // free(page);
  
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
  struct _u_compressed_inmemory_website_config file_config;

  Database database = {0};

  loadParams("properties/properties.json", &database);

  static_list = List_create(100, sizeof(Person));

  Database_init(database.hostname, database.username, database.password, database.database);

  if (u_init_compressed_inmemory_website_config(&file_config) == U_OK) {
    u_map_put(&file_config.mime_types, ".html", "text/html");
    u_map_put(&file_config.mime_types, ".css", "text/css");
    u_map_put(&file_config.mime_types, ".js", "application/javascript");
    u_map_put(&file_config.mime_types, ".png", "image/png");
    u_map_put(&file_config.mime_types, ".jpg", "image/jpeg");
    u_map_put(&file_config.mime_types, ".jpeg", "image/jpeg");
    u_map_put(&file_config.mime_types, ".ttf", "font/ttf");
    u_map_put(&file_config.mime_types, ".woff", "font/woff");
    u_map_put(&file_config.mime_types, ".woff2", "font/woff2");
    u_map_put(&file_config.mime_types, ".map", "application/octet-stream");
    u_map_put(&file_config.mime_types, ".json", "application/json");
    u_map_put(&file_config.mime_types, "*", "application/octet-stream");
    file_config.files_path = "static";
    file_config.url_prefix = PREFIX_STATIC;
  } else{
    return EXIT_FAILURE;
  }

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
  ulfius_add_endpoint_by_val(&instance, "POST", "/update", NULL, 0, &callback_update, NULL);
  ulfius_add_endpoint_by_val(&instance, "POST", "/insert", NULL, 1, &callback_insert, NULL);
  ulfius_add_endpoint_by_val(&instance, "POST", "/delete", NULL, 0, &callback_delete, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", "/send", NULL, 0, &callback_send_data, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", PREFIX_STATIC, "*", 0, &callback_static_compressed_inmemory_website, &file_config);
  

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
  u_clean_compressed_inmemory_website_config(&file_config);
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

int callback_static_compressed_inmemory_website (const struct _u_request * request, struct _u_response * response, void * user_data) {
  struct _u_compressed_inmemory_website_config * config = (struct _u_compressed_inmemory_website_config *)user_data;
  char ** accept_list = NULL;
  int ret = U_CALLBACK_CONTINUE, compress_mode = U_COMPRESS_NONE, res;
  z_stream defstream;
  unsigned char * file_content, * file_content_orig = NULL;
  size_t length, read_length, offset, data_zip_len = 0;
  FILE * f;
  char * file_requested, * file_path, * url_dup_save, * data_zip = NULL;
  const char * content_type;

  /*
   * Comment this if statement if you don't access static files url from root dir, like /app
   */
  if (request->callback_position > 0) {
    return U_CALLBACK_IGNORE;
  } else {
    file_requested = o_strdup(request->http_url);
    url_dup_save = file_requested;

    while (file_requested[0] == '/') {
      file_requested++;
    }
    file_requested += o_strlen((config->url_prefix));
    while (file_requested[0] == '/') {
      file_requested++;
    }

    if (strchr(file_requested, '#') != NULL) {
      *strchr(file_requested, '#') = '\0';
    }

    if (strchr(file_requested, '?') != NULL) {
      *strchr(file_requested, '?') = '\0';
    }

    if (file_requested == NULL || o_strlen(file_requested) == 0 || 0 == o_strcmp("/", file_requested)) {
      o_free(url_dup_save);
      url_dup_save = file_requested = o_strdup("index.html");
    }

    if (!u_map_has_key_case(response->map_header, U_CONTENT_HEADER)) {
      if (split_string(u_map_get_case(request->map_header, U_ACCEPT_HEADER), ",", &accept_list)) {
        if (config->allow_gzip && string_array_has_trimmed_value((const char **)accept_list, U_ACCEPT_GZIP)) {
          compress_mode = U_COMPRESS_GZIP;
        } else if (config->allow_deflate && string_array_has_trimmed_value((const char **)accept_list, U_ACCEPT_DEFLATE)) {
          compress_mode = U_COMPRESS_DEFL;
        }

        content_type = u_map_get_case(&config->mime_types, get_filename_ext(file_requested));
        if (content_type == NULL) {
          content_type = u_map_get(&config->mime_types, "*");
        }
        if (!string_array_has_value((const char **)config->mime_types_compressed, content_type)) {
          compress_mode = U_COMPRESS_NONE;
        }

        u_map_put(response->map_header, "Content-Type", content_type);
        u_map_copy_into(response->map_header, &config->map_header);

        if (compress_mode != U_COMPRESS_NONE) {
          if (compress_mode == U_COMPRESS_GZIP && config->allow_cache_compressed && u_map_has_key(&config->gzip_files, file_requested)) {
            ulfius_set_binary_body_response(response, 200, u_map_get(&config->gzip_files, file_requested), u_map_get_length(&config->gzip_files, file_requested));
            u_map_put(response->map_header, U_CONTENT_HEADER, U_ACCEPT_GZIP);
          } else if (compress_mode == U_COMPRESS_DEFL && config->allow_cache_compressed && u_map_has_key(&config->deflate_files, file_requested)) {
            ulfius_set_binary_body_response(response, 200, u_map_get(&config->deflate_files, file_requested), u_map_get_length(&config->deflate_files, file_requested));
            u_map_put(response->map_header, U_CONTENT_HEADER, U_ACCEPT_DEFLATE);
          } else {
            file_path = msprintf("%s/%s", ((struct _u_compressed_inmemory_website_config *)user_data)->files_path, file_requested);

            if (access(file_path, F_OK) != -1) {
              if (!pthread_mutex_lock(&config->lock)) {
                f = fopen (file_path, "rb");
                if (f) {
                  fseek (f, 0, SEEK_END);
                  offset = length = ftell (f);
                  fseek (f, 0, SEEK_SET);

                  if ((file_content_orig = file_content = o_malloc(length)) != NULL && (data_zip = o_malloc((2*length)+20)) != NULL) {
                    defstream.zalloc = u_zalloc;
                    defstream.zfree = u_zfree;
                    defstream.opaque = Z_NULL;
                    defstream.avail_in = (uInt)length;
                    defstream.next_in = (Bytef *)file_content;
                    while ((read_length = fread(file_content, sizeof(char), offset, f))) {
                      file_content += read_length;
                      offset -= read_length;
                    }

                    if (compress_mode == U_COMPRESS_GZIP) {
                      if (deflateInit2(&defstream, 
                                       Z_DEFAULT_COMPRESSION, 
                                       Z_DEFLATED,
                                       U_GZIP_WINDOW_BITS | U_GZIP_ENCODING,
                                       8,
                                       Z_DEFAULT_STRATEGY) != Z_OK) {
                        ret = U_CALLBACK_ERROR;
                      }
                    } else {
                      if (deflateInit(&defstream, Z_BEST_COMPRESSION) != Z_OK) {
                        ret = U_CALLBACK_ERROR;
                      }
                    }
                    if (ret == U_CALLBACK_CONTINUE) {
                      do {
                        if ((data_zip = o_realloc(data_zip, data_zip_len+_U_W_BLOCK_SIZE)) != NULL) {
                          defstream.avail_out = _U_W_BLOCK_SIZE;
                          defstream.next_out = ((Bytef *)data_zip)+data_zip_len;
                          switch ((res = deflate(&defstream, Z_FINISH))) {
                            case Z_OK:
                            case Z_STREAM_END:
                            case Z_BUF_ERROR:
                              break;
                            default:
                              ret = U_CALLBACK_ERROR;
                              break;
                          }
                          data_zip_len += _U_W_BLOCK_SIZE - defstream.avail_out;
                        } else {
                          ret = U_CALLBACK_ERROR;
                        }
                      } while (U_CALLBACK_CONTINUE == ret && defstream.avail_out == 0);
                      
                      if (ret == U_CALLBACK_CONTINUE) {
                        if (compress_mode == U_COMPRESS_GZIP) {
                          if (config->allow_cache_compressed) {
                            u_map_put_binary(&config->gzip_files, file_requested, data_zip, 0, defstream.total_out);
                          }
                          ulfius_set_binary_body_response(response, 200, u_map_get(&config->gzip_files, file_requested), u_map_get_length(&config->gzip_files, file_requested));
                        } else {
                          if (config->allow_cache_compressed) {
                            u_map_put_binary(&config->deflate_files, file_requested, data_zip, 0, defstream.total_out);
                          }
                          ulfius_set_binary_body_response(response, 200, u_map_get(&config->deflate_files, file_requested), u_map_get_length(&config->deflate_files, file_requested));
                        }
                        u_map_put(response->map_header, U_CONTENT_HEADER, compress_mode==U_COMPRESS_GZIP?U_ACCEPT_GZIP:U_ACCEPT_DEFLATE);
                      }
                    }
                    deflateEnd(&defstream);
                    o_free(data_zip);
                  } else {
                    ret = U_CALLBACK_ERROR;
                  }
                  o_free(file_content_orig);
                  fclose(f);
                }
                pthread_mutex_unlock(&config->lock);
              } else {
                ret = U_CALLBACK_ERROR;
              }
            } else {
              if (((struct _u_compressed_inmemory_website_config *)user_data)->redirect_on_404 == NULL) {
                ret = U_CALLBACK_IGNORE;
              } else {
                ulfius_add_header_to_response(response, "Location", ((struct _u_compressed_inmemory_website_config *)user_data)->redirect_on_404);
                response->status = 302;
              }
            }
            o_free(file_path);
          }
        } else {
          ret = callback_static_file_uncompressed(request, response, user_data);
        }
        free_string_array(accept_list);
      }
    }
    o_free(url_dup_save);
  }

  return ret;
}