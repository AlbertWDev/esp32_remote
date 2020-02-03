#include "storage.h"

#define FILE_BUFFER_SIZE 1024

esp_err_t _get_node_name_from_uri(const char* uri, char* node_name, size_t max_len) {
    size_t uri_len = strlen(uri);
    size_t offset = strlen("/v1/storage");

    if(offset > uri_len)
        return RMGMT_ERR_STORAGE_NODE_INVALID_NAME;
    
    if(offset == uri_len)
        strcpy(node_name, "/"); // Root directory
    else {
        strncpy(node_name, uri + offset, max_len - 1);
        node_name[max_len - 1] = '\0';
    }
    return ESP_OK;
}

static const char* _entry_type_to_str(uint8_t type) {
    switch(type) {
        case DT_DIR:
            return "DIR";
        case DT_REG:
            return "FILE";
        default:
            return "UNKNOWN";
    }
}

cJSON* _get_dir_entry_json(struct dirent* entry) {
    cJSON* entry_json = cJSON_CreateObject();
    
    //cJSON_AddNumberToObject(entry_json, "inode", entry->d_ino);
    cJSON_AddStringToObject(entry_json, "name", entry->d_name);
    cJSON_AddStringToObject(entry_json, "type", _entry_type_to_str(entry->d_type));

    return entry_json;
}

cJSON* _list_dir_json(char* path, size_t path_len) {
    DIR* dir = opendir(path);
    if (!dir) return NULL;

    cJSON *dir_list = cJSON_CreateArray();
    if(dir_list == NULL) return NULL;
    
    char filepath[RMGMT_STORAGE_NODE_NAME_MAX_LENGTH];
    struct dirent *entry;
    struct stat filestat;
    while ((entry = readdir(dir)) != NULL) {
        cJSON* entry_json = _get_dir_entry_json(entry);

        // Get full file path
        sprintf(filepath, path);
        if(path[path_len-1] != '/') strcat(filepath, "/");
        strcat(filepath, entry->d_name);

        // Add extra info
        if(stat(filepath, &filestat) == 0) {
            cJSON_AddNumberToObject(entry_json, "mode", filestat.st_mode);
            //cJSON_AddNumberToObject(entry_json, "created", filestat.st_ctime);1
            //cJSON_AddNumberToObject(entry_json, "accessed", filestat.st_atime);
            cJSON_AddNumberToObject(entry_json, "modified", filestat.st_mtime);
            cJSON_AddNumberToObject(entry_json, "size", filestat.st_size);
        }
        
        cJSON_AddItemToArray(dir_list, entry_json);
    }
    closedir(dir);
    return dir_list;
}

cJSON* _list_mount_points_json() {
    ESP_LOGW("RMGMT_STORAGE", "esp-idf doesn't provide a way to know registered filesystems, using 'spiffs' and 'sdcard' as default");

    cJSON* storage_json = cJSON_CreateArray();
    if(storage_json == NULL) return NULL;

    cJSON* mount_point_json = cJSON_CreateObject();
    cJSON_AddStringToObject(mount_point_json, "name", "spiffs");
    cJSON_AddStringToObject(mount_point_json, "type", _entry_type_to_str(DT_DIR));
    cJSON_AddItemToArray(storage_json, mount_point_json);

    mount_point_json = cJSON_CreateObject();
    cJSON_AddStringToObject(mount_point_json, "name", "sdcard");
    cJSON_AddStringToObject(mount_point_json, "type", _entry_type_to_str(DT_DIR));
    cJSON_AddItemToArray(storage_json, mount_point_json);

    return storage_json;
}

esp_err_t _rmgmt_get_storage(httpd_req_t *req) {
    APPLY_HEADERS(req);
    
    cJSON* storage_json = _list_mount_points_json();
    if(storage_json == NULL) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Mount points not found or not accessible");
        return ESP_OK;
    }
    
    httpd_resp_set_type(req, "application/json");
    const char *json = cJSON_Print(storage_json);

    httpd_resp_sendstr(req, json);

    free((void *)json);
    cJSON_Delete(storage_json);
    return ESP_OK;
}

esp_err_t _rmgmt_get_storage_node(httpd_req_t *req) {
    APPLY_HEADERS(req);
    esp_err_t ret;
    
    // Get node path from URI
    char node_name[RMGMT_STORAGE_NODE_NAME_MAX_LENGTH];
    ret = _get_node_name_from_uri(req->uri, node_name, RMGMT_STORAGE_NODE_NAME_MAX_LENGTH);
    if(ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid node name");
        return ESP_FAIL;
    }

    size_t node_name_len = strlen(node_name);
    if(node_name[node_name_len-1] == '/') { // Directory
        if(node_name_len > 1) node_name[--node_name_len] = '\0';    // Remove ending '/'

        cJSON* dir_json = _list_dir_json(node_name, node_name_len);
        if(dir_json == NULL) {
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Directory not found ot not accessible");
            return ESP_OK;
        }
        
        const char *json = cJSON_Print(dir_json);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, json);

        free((void *)json);
        cJSON_Delete(dir_json);
    } else { 
        int fd = open(node_name, O_RDONLY, 0);
        if (fd == -1) {
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found or not accessible");
            return ESP_OK;
        }

        httpd_resp_set_type(req, "application/octet-stream");

        char *chunk = (char*)malloc(FILE_BUFFER_SIZE);
        ssize_t read_bytes;

        while((read_bytes = read(fd, chunk, FILE_BUFFER_SIZE)) > 0)
            if((ret = httpd_resp_send_chunk(req, chunk, read_bytes)) != ESP_OK) break;

        free(chunk);
        close(fd);
        httpd_resp_send_chunk(req, NULL, 0);
        
        if(read_bytes < 0 || ret != ESP_OK)
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
    }
    return ESP_OK;
}

esp_err_t _rmgmt_put_storage_node(httpd_req_t *req) {
    APPLY_HEADERS(req);
    esp_err_t ret;    
    
    // Get node path from URI
    char node_name[RMGMT_STORAGE_NODE_NAME_MAX_LENGTH];
    ret = _get_node_name_from_uri(req->uri, node_name, RMGMT_STORAGE_NODE_NAME_MAX_LENGTH);
    if(ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid node name");
        return ESP_FAIL;
    }

    size_t node_name_len = strlen(node_name);
    if(node_name[node_name_len-1] == '/') { // Directory
        node_name[node_name_len-1] = '\0';  // Remove trailing slash

        if(mkdir(node_name, 0755) == 0)
            httpd_resp_sendstr(req, "Directory created");
        else
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Couldn't create directory");
    } else {
        // Open file in write mode (overwrite if existed)
        FILE* f = fopen(node_name, "w");
        if (f == NULL) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Can't create file");
            return ESP_OK;
        }

        // Write file content in chunks as received from client
        char* chunk = (char*)malloc(FILE_BUFFER_SIZE);
        int received, total_received = 0;
        while(total_received < req->content_len) {
            total_received += (received = httpd_req_recv(req, chunk, FILE_BUFFER_SIZE));
            if(received <= 0) {
                free(chunk);
                fclose(f);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Couldn't receive file content");
                return ESP_OK;
            }
            if(fwrite(chunk, sizeof(char), received, f) != received) {
                free(chunk);
                fclose(f);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Couldn't write content to file");
                return ESP_OK;
            }
        }
        free(chunk);
        fclose(f);

        httpd_resp_sendstr(req, "File created");
    }
    return ESP_OK;
}

esp_err_t _rmgmt_post_storage_node(httpd_req_t *req) {
    APPLY_HEADERS(req);

    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Not implemented");
    return ESP_OK;
}

esp_err_t _rmgmt_delete_storage_node(httpd_req_t *req) {
    APPLY_HEADERS(req);
    esp_err_t ret;
    
    // Get node path from URI
    char node_name[RMGMT_STORAGE_NODE_NAME_MAX_LENGTH];
    ret = _get_node_name_from_uri(req->uri, node_name, RMGMT_STORAGE_NODE_NAME_MAX_LENGTH);
    if(ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid node name");
        return ESP_FAIL;
    }

    size_t node_name_len = strlen(node_name);
    if(node_name[node_name_len-1] == '/') { // Directory
        node_name[node_name_len-1] = '\0';  // Remove trailing slash

        if(rmdir(node_name) == 0)
            httpd_resp_sendstr(req, "Directory deleted");
        else
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Couldn't delete directory");
    } else {
        if(remove(node_name) == 0)
            httpd_resp_sendstr(req, "File deleted");
        else
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Couldn't delete file"); 
    }
    return ESP_OK;
}