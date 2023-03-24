#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>

struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL) {
    printf("Not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

void searchHashtag(const char *hashtag) {
  CURL *curl_handle;
  CURLcode res;

  struct MemoryStruct chunk;

  chunk.memory = malloc(1);
  chunk.size = 0;

  curl_global_init(CURL_GLOBAL_ALL);
  curl_handle = curl_easy_init();

  if (!curl_handle) {
    fprintf(stderr, "Error initializing curl\n");
    return;
  }

  char url[256];
  snprintf(url, sizeof(url), "https://www.tiktok.com/node/share/tag/%s", hashtag);

  curl_easy_setopt(curl_handle, CURLOPT_URL, url);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "User-Agent: seu_user_agent");
  curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

  res = curl_easy_perform(curl_handle);

  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
  } else {
    json_object *root_obj = json_tokener_parse(chunk.memory);
    json_object *item_list = NULL;

    if (json_object_object_get_ex(root_obj, "itemList", &item_list)) {
      int num_videos = json_object_array_length(item_list);

      for (int i = 0; i < num_videos; i++) {
        json_object *video = json_object_array_get_idx(item_list, i);
        json_object *video_id_obj;
        json_object *desc_obj;
        json_object *author_obj;
        json_object *unique_id_obj;

        json_object_object_get_ex(video, "id", &video_id_obj);
        json_object_object_get_ex(video, "desc", &desc_obj);
        json_object_object_get_ex(video, "author", &author_obj);
        json_object_object_get_ex(author_obj, "uniqueId", &unique_id_obj);

        printf("Video %d:\n", i + 1);
        printf("ID: %s\n", json_object_get_string(video_id_obj));
        printf("Descrição: %s\n", json_object_get_string(desc_obj));
        printf("URL: https://www.tiktok.com/@%s/video/%s\n",
               json_object_get_string(unique_id_obj), json_object_get_string(video_id_obj));
        printf("-----------------------------\n");
      }
    } else {
      fprintf(stderr, "Error parsing JSON response\n");
    }

    json_object_put(root_obj);
  }

  curl_easy_cleanup(curl_handle);
  curl_global_cleanup();

  free(chunk.memory);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s hashtag\n", argv[0]);
    return 1;
  }

  searchHashtag(argv[1]);
  return 0;
}
