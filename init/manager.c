#include "../include/init/manager.h"
#include "../include/kernel/log.h"
#include "../include/drivers/console.h"

static void strcpy_safe(char *dest, const char *src, size_t max_len) {
    size_t i = 0;
    while (i < max_len - 1 && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void init_manager_init(init_manager_t *manager) {
    if (!manager) {
        return;
    }
    
    manager->service_count = 0;
    manager->all_started = 0;
    
    for (int i = 0; i < MAX_SERVICES; i++) {
        manager->services[i].name[0] = '\0';
        manager->services[i].start = 0;
        manager->services[i].dependency_count = 0;
        manager->services[i].failure_policy = FAILURE_POLICY_IGNORE;
        manager->services[i].status = SERVICE_STATUS_STOPPED;
        manager->services[i].error_code = 0;
        
        for (int j = 0; j < MAX_DEPENDENCIES; j++) {
            manager->services[i].dependencies[j] = 0;
        }
    }
}

int init_manager_register_service(init_manager_t *manager,
                                   const char *name,
                                   service_start_fn start,
                                   const char **dependencies,
                                   int dependency_count,
                                   failure_policy_t policy) {
    if (!manager || !name || !start) {
        return -1;
    }
    
    if (manager->service_count >= MAX_SERVICES) {
        return -1;
    }
    
    service_descriptor_t *svc = &manager->services[manager->service_count];
    strcpy_safe(svc->name, name, SERVICE_NAME_MAX);
    svc->start = start;
    svc->failure_policy = policy;
    svc->status = SERVICE_STATUS_STOPPED;
    svc->error_code = 0;
    svc->dependency_count = 0;
    
    if (dependencies && dependency_count > 0) {
        for (int i = 0; i < dependency_count && i < MAX_DEPENDENCIES; i++) {
            svc->dependencies[i] = dependencies[i];
            svc->dependency_count++;
        }
    }
    
    manager->service_count++;
    return 0;
}

service_descriptor_t *init_manager_get_service(init_manager_t *manager, const char *name) {
    if (!manager || !name) {
        return 0;
    }
    
    for (int i = 0; i < manager->service_count; i++) {
        if (strcmp_impl(manager->services[i].name, name) == 0) {
            return &manager->services[i];
        }
    }
    
    return 0;
}

static int service_can_start(init_manager_t *manager, service_descriptor_t *svc) {
    if (!manager || !svc) {
        return 0;
    }
    
    for (int i = 0; i < svc->dependency_count; i++) {
        const char *dep_name = svc->dependencies[i];
        if (!dep_name) {
            continue;
        }
        
        service_descriptor_t *dep = init_manager_get_service(manager, dep_name);
        if (!dep || dep->status != SERVICE_STATUS_RUNNING) {
            return 0;
        }
    }
    
    return 1;
}

static int start_service(init_manager_t *manager, service_descriptor_t *svc) {
    if (!manager || !svc) {
        return -1;
    }
    
    if (svc->status == SERVICE_STATUS_RUNNING) {
        return 0;
    }
    
    if (!service_can_start(manager, svc)) {
        return -2;
    }
    
    char log_msg[128];
    strcpy_safe(log_msg, "Starting service: ", sizeof(log_msg));
    size_t len = strlen_impl(log_msg);
    strcpy_safe(log_msg + len, svc->name, sizeof(log_msg) - len);
    KLOG_INFO("init", log_msg);
    
    console_print("  Starting ");
    console_print(svc->name);
    console_print("... ");
    
    svc->status = SERVICE_STATUS_STARTING;
    int result = svc->start(svc);
    
    if (result == 0) {
        svc->status = SERVICE_STATUS_RUNNING;
        console_print("OK\n");
        
        strcpy_safe(log_msg, "Service started: ", sizeof(log_msg));
        len = strlen_impl(log_msg);
        strcpy_safe(log_msg + len, svc->name, sizeof(log_msg) - len);
        KLOG_INFO("init", log_msg);
    } else {
        svc->status = SERVICE_STATUS_FAILED;
        svc->error_code = result;
        console_print("FAILED\n");
        
        strcpy_safe(log_msg, "Service failed: ", sizeof(log_msg));
        len = strlen_impl(log_msg);
        strcpy_safe(log_msg + len, svc->name, sizeof(log_msg) - len);
        KLOG_ERROR("init", log_msg);
        
        if (svc->failure_policy == FAILURE_POLICY_HALT) {
            KLOG_ERROR("init", "Critical service failed, halting");
            return -1;
        }
    }
    
    return result;
}

int init_manager_start_all(init_manager_t *manager) {
    if (!manager) {
        return -1;
    }
    
    KLOG_INFO("init", "Starting all services");
    console_print("Initializing services:\n");
    
    int started[MAX_SERVICES];
    for (int i = 0; i < MAX_SERVICES; i++) {
        started[i] = 0;
    }
    
    int total_started = 0;
    int max_iterations = manager->service_count * 2;
    int iteration = 0;
    
    while (total_started < manager->service_count && iteration < max_iterations) {
        iteration++;
        int progress = 0;
        
        for (int i = 0; i < manager->service_count; i++) {
            if (started[i]) {
                continue;
            }
            
            service_descriptor_t *svc = &manager->services[i];
            if (service_can_start(manager, svc)) {
                int result = start_service(manager, svc);
                started[i] = 1;
                total_started++;
                progress = 1;
                
                if (result != 0 && svc->failure_policy == FAILURE_POLICY_HALT) {
                    return -1;
                }
            }
        }
        
        if (!progress) {
            break;
        }
    }
    
    if (total_started < manager->service_count) {
        KLOG_WARN("init", "Some services could not be started (unmet dependencies)");
        console_print("Warning: Some services could not be started\n");
        return -2;
    }
    
    manager->all_started = 1;
    KLOG_INFO("init", "All services started successfully");
    console_print("All services initialized successfully\n");
    
    return 0;
}
