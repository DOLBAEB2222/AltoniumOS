#ifndef INIT_MANAGER_H
#define INIT_MANAGER_H

#include "../lib/string.h"

#define MAX_SERVICES 16
#define MAX_DEPENDENCIES 8
#define SERVICE_NAME_MAX 32

typedef enum {
    SERVICE_STATUS_STOPPED = 0,
    SERVICE_STATUS_STARTING,
    SERVICE_STATUS_RUNNING,
    SERVICE_STATUS_FAILED
} service_status_t;

typedef enum {
    FAILURE_POLICY_IGNORE = 0,
    FAILURE_POLICY_WARN,
    FAILURE_POLICY_HALT
} failure_policy_t;

typedef struct service_descriptor service_descriptor_t;

typedef int (*service_start_fn)(service_descriptor_t *service);

struct service_descriptor {
    char name[SERVICE_NAME_MAX];
    service_start_fn start;
    const char *dependencies[MAX_DEPENDENCIES];
    int dependency_count;
    failure_policy_t failure_policy;
    service_status_t status;
    int error_code;
};

typedef struct {
    service_descriptor_t services[MAX_SERVICES];
    int service_count;
    int all_started;
} init_manager_t;

void init_manager_init(init_manager_t *manager);
int init_manager_register_service(init_manager_t *manager, 
                                   const char *name,
                                   service_start_fn start,
                                   const char **dependencies,
                                   int dependency_count,
                                   failure_policy_t policy);
int init_manager_start_all(init_manager_t *manager);
service_descriptor_t *init_manager_get_service(init_manager_t *manager, const char *name);

#endif
